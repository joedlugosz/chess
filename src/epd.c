/*
 * Functions which run Extended Position Description (EPD) test cases
 *
 * EPD specifies a test case as a line of text containing a position to be
 * searched in PGN, with multiple commands. Some commands have actions which are
 * executed before the search, some have actions after the search, and some have
 * both. Currently, only a few commands are implemented. Commands return a PASS
 * or FAIL result, and an overall PASS/FAIL is recorded for each case.
 *
 * Cases can be identified and organised into sets by the "id" command which
 * specifies the name of the set and the number of the case within the set, e.g.
 * `id "BK.01";` Results are organised by set.
 */
#include <ctype.h>
#include <malloc.h>
#include <math.h>
#include <stdio.h>

#include "fen.h"
#include "history.h"
#include "io.h"
#include "position.h"
#include "search.h"

enum {
  EPD_CMD_LENGTH_MAX = 1000,
  EPD_N_CMD_MAX = 100,
  MAX_SET = 100,
  MAX_SET_NAME = 100,
  MAX_OP_NAME = 100,
};

/* Array holding parsed input commands */
char epd_cmd[EPD_N_CMD_MAX][EPD_CMD_LENGTH_MAX];

/* Array holding arguments for input commands */
char *epd_arg[EPD_N_CMD_MAX];

/* Array holding ID strings of cases */
char id[EPD_N_CMD_MAX];

/* Output of cases */
char output[1100];

int show_pass = 1;
int show_fail = 1;
int show_total = 1;
int show_stats = 1;
int show_board = 0;
int show_thought = 0;

/* Target value for "dm" moves */
int direct_mate = 0;
/* Number of fullmoves to mate */
int full_move = 0;

struct epd_stat {
  double total;
  double max;
  double min;
  double mean;
  double variance;
  double std_dev;
};
const int n_stats = sizeof(struct epd_stat) / sizeof(double);

const char stat_name[][20] = {
    "\u03a3",                            /* Total */
    "max",    "min", "mean", "\u03c3^2", /* Variance */
    "\u03c3",                            /* Std dev */
};

typedef double (*var_get_func)(const struct search_result *);

struct epd_var {
  const char name[20];
  const char format[10];
  var_get_func get;
};

static double get_n_leaf(const struct search_result *r) {
  return (double)r->n_leaf / 1000.0;
}
static double get_n_node(const struct search_result *r) {
  return (double)r->n_node / 1000.0;
}
static double get_n_check_node(const struct search_result *r) {
  return (double)r->n_check_moves / 1000.0;
}
static double get_r_check_node(const struct search_result *r) {
  return (double)r->n_check_moves / (double)r->n_node;
}
static double get_branching_factor(const struct search_result *r) {
  return r->branching_factor;
}
static double get_time(const struct search_result *r) { return r->time; }

const struct epd_var vars[] = {
    {"time (s)", "%16.2lf", get_time},
    {"branching factor", "%16.2lf", get_branching_factor},
    {"n_leaf (k)", "%16.0lf", get_n_leaf},
    {"n_node (k)", "%16.0lf", get_n_node},
    {"n_check_node (k)", "%16.0lf", get_n_check_node},
    {"r_check_node", "%16.2lf", get_r_check_node},
};
const int n_vars = sizeof(vars) / sizeof(vars[0]);

/* A set of EPD cases */
struct epd_set {
  char name[MAX_SET_NAME];
  int n_pass;
  int n_total;
  //  struct search_result set_total;
};

struct epd_stat *stats;

/* List of all EPD sets */
struct epd_set epd_sets[MAX_SET];

/* The total number of EPD sets recorded so far */
int n_sets;

/* The set of the current command */
char current_set[MAX_SET_NAME];

struct search_result *search_results;
int *id_set;

/* Add a new set of EPD cases to the global list */
int add_set(const char *name) {
  strcpy(epd_sets[n_sets].name, name);
  int ret = n_sets++;
  return ret;
}

/* Find an existing set of EPD cases in the global list */
int find_set(const char *name) {
  for (int i = 0; i < n_sets; i++) {
    if (strcmp(epd_sets[i].name, name) == 0) return i;
  }
  return -1;
}

/* Add an EPD test result for a test within a set */
static void add_result(const char *name, int pass, int id) {
  int set_id = find_set(name);
  if (set_id == -1) set_id = add_set(name);
  if (set_id != -1) {
    struct epd_set *set = &epd_sets[set_id];
    if (pass) set->n_pass++;
    set->n_total++;
    id_set[id] = set_id;
  }
}

/* Print the total pass/fail results by set */
static void print_results(void) {
  for (int i = 0; i < n_sets; i++) {
    printf("   %-60s %d/%d %0.2lf%%\n", epd_sets[i].name, epd_sets[i].n_pass,
           epd_sets[i].n_total,
           (double)epd_sets[i].n_pass / (double)epd_sets[i].n_total * 100.0);
  }
}

/* "bm" <MOVE> - best move is MOVE - FAIL if search result is different */
static int epd_bm(char *args, struct search_result *result) {
  char san[10];
  format_move_san(san, &result->move);

  char *bm = strtok(args, " ");
  while (bm) {
    if (strcmp(san, bm) == 0) return 0;
    bm = strtok(0, " ");
  }

  char buf[100];
  sprintf(buf, "actual: %s", san);
  strcat(output, buf);
  return 1;
}

/* "dm" <N> - direct mate in N moves - read the argument */
static int epd_dm_pre(char *args, struct search_result *result) {
  if (sscanf(args, "%d", &direct_mate) != 1) return 1;
  return 0;
}

/* "dm" <N> - direct mate in N moves - FAIL if search result > N */
static int epd_dm_post(char *args, struct search_result *result) {
  return (full_move > direct_mate) ? 1 : 0;
}

/* "id" - set id of case */
static int epd_id(char *args, struct search_result *result) {
  char *src = args;
  while (isspace(*src)) src++;
  if (*src == '\"') src++;
  src = strtok(src, "\"");
  strcpy(&id[0], src);
  char *ptr = src + strlen(src);
  while (*--ptr != '.' && ptr > src)
    ;
  if (*ptr == '.') *ptr = 0;
  char *set_name = src;
  strcpy(&current_set[0], set_name);
  sprintf(output, "%-20s ", id);
  return 0;
}

/* Handle EPD comments with no effect */
static int epd_comment(char *args, struct search_result *result) { return 0; }

/* EPD command function */
typedef int (*epd_fn)(char *, struct search_result *);

/* Entry in a table of EPD command definitions */
struct epd_cmd {
  char name[MAX_OP_NAME];
  epd_fn fn;
};

/* Zero-terminated table of EPD command definitions to be executed post-search
 */
struct epd_cmd epd_cmds_post[] = {
    {"bm", epd_bm},
    {"dm", epd_dm_post},
    {"#", epd_comment},
    {"", 0},
};

/* Zero-terminated table of EPD command definitions to be executed pre-search */
struct epd_cmd epd_cmds_pre[] = {
    {"dm", epd_dm_pre},
    {"id", epd_id},
    {"", 0},
};

/* Find an command function from a table */
static epd_fn epd_find_cmd(const char *name, struct epd_cmd *epd_cmds) {
  struct epd_cmd *cmd = epd_cmds;

  while (cmd->name[0]) {
    if (strcmp(cmd->name, name) == 0) return (cmd->fn);
    cmd++;
  }

  /* Not found */
  return 0;
}

/* Run an EPD case */
int epd_run(const char *epd_line, int depth, int id) {
  struct position position;
  int pass = 1;
  direct_mate = 0;

  char *line_buf = (char *)malloc(strlen(epd_line) + 1);
  strcpy(line_buf, epd_line);

  char *placement_text = strtok(line_buf, " ");
  char *active_player_text = strtok(0, " ");
  char *castling_text = strtok(0, " ");
  char *en_passant_text = strtok(0, " ");

  load_fen(&position, placement_text, active_player_text, castling_text,
           en_passant_text, "0", "1");

  if (show_board) print_board(&position, 0, 0);

  /* Parse remainder of line into list of EPD commands, delimited by ';' */
  char *op;
  int n_ops = 0;
  while ((op = strtok(0, ";"))) {
    if (!op || !*op) break;
    while (*op == ' ') op++;
    if (*op == '#') break;
    strcpy(&epd_cmd[n_ops][0], op);
    n_ops++;
  }

  /* Parse each EPD command into command and arguments, delimited by ' ' */
  for (int i = 0; i < n_ops; i++) {
    strtok(epd_cmd[i], " ");
    epd_arg[i] = strtok(0, "");
  }

  output[0] = 0;
  current_set[0] = 0;

  /* Match and execute commands that should be executed before search */
  for (int i = 0; i < n_ops; i++) {
    char *name = epd_cmd[i];
    if (!epd_cmd[i][0]) break;
    epd_fn fn;
    if ((fn = epd_find_cmd(name, epd_cmds_pre))) (*fn)(epd_arg[i], 0);
  }

  /* Do the search */
  struct history history;
  memset(&history, 0, sizeof(history));
  full_move = 0;

  struct search_result first_result;
  struct search_result result;
  if (direct_mate == 0) {
    search(depth, 0.0, 0.0, &history, &position, &result, show_board);
    memcpy(&first_result, &result, sizeof(first_result));
  } else {
    while (full_move <= direct_mate) {
      search(depth, 0.0, 0.0, &history, &position, &result, show_board);
      if (full_move == 0) memcpy(&first_result, &result, sizeof(first_result));
      if (result.move.result & MATE) break;
      make_move(&position, &result.move);
      change_player(&position);
      search(depth, 0.0, 0.0, &history, &position, &result, show_thought);
      if (result.move.result & MATE) break;
      make_move(&position, &result.move);
      change_player(&position);
      full_move++;
    }
  }

  memcpy(&search_results[id], &first_result, sizeof(search_results[0]));

  /* Match and execute commands that should be executed after search */
  for (int i = 0; i < n_ops; i++) {
    char *name = epd_cmd[i];
    if (!epd_cmd[i][0]) break;
    epd_fn fn;
    if ((fn = epd_find_cmd(name, epd_cmds_post))) {
      char buf[100100];
      if (snprintf(buf, sizeof(buf) - 1, "%s %s ", epd_cmd[i], epd_arg[i]) < 0)
        ;
      strcat(output, buf);
      if ((*fn)(epd_arg[i], &first_result)) {
        strcat(output, " FAIL; ");
        pass = 0;
      } else {
        strcat(output, " PASS; ");
      }
    }
  }

  free(line_buf);

  return pass;
}

/* Run tests from a file of EPD positions. Each line of the file contains
   and EPD case */
int epd_test(const char *filename, int depth) {
  FILE *f = fopen(filename, "r");
  if (!f) {
    char buf[1000];
    sprintf(buf, "Opening epd file %s", filename);
    perror(buf);
    return 1;
  }

  char epd_line[1000];
  int n_tests = 0;
  while (fgets(epd_line, sizeof(epd_line), f)) {
    if (*epd_line == '#') continue;
    n_tests++;
  }
  fseek(f, SEEK_SET, 0);

  search_results =
      (struct search_result *)malloc(n_tests * sizeof(struct search_result));
  if (search_results == 0) return 1;
  id_set = (int *)malloc(n_tests * sizeof(int));
  if (id_set == 0) return 1;
  for (int i = 0; i < n_tests; i++) id_set[i] = -1;

  if (show_stats) {
    stats = (struct epd_stat *)calloc(n_sets * n_vars, sizeof(struct epd_stat));
    if (stats == 0) return 1;
  }

  int index = 0;
  int n_pass = 0;
  int id = 0;
  while (fgets(epd_line, sizeof(epd_line), f)) {
    if (*epd_line == '#') continue;
    int pass = epd_run(epd_line, depth, id);
    index++;
    char buf[2000];
    sprintf(buf, "%d/%d %s", index, n_tests, output);

    if (!pass) n_pass++;

    if (pass || show_pass) {
      printf("%s\n", buf);
    }

    /* current_set is set by epd_id */
    if (current_set[0]) add_result(current_set, pass, id);
    id++;
  }

  fclose(f);

  if (show_total) {
    printf("\nResults by category:\n");
    print_results();
    printf("\nTotal:\n");
    // printf("   %d/%d %0.2lf%%\n", n_pass, n_tests,
    //        (double)n_pass / (double)n_tests);
  }

  if (show_stats) {
    for (int i = 0; i < n_tests; i++) {
      int set_id = id_set[i];
      if (set_id == -1) continue;
      for (int j = 0; j < n_vars; j++) {
        struct epd_stat *stat = &stats[set_id * n_vars + j];
        stat->total = 0.0;
        stat->max = -INFINITY;
        stat->min = INFINITY;
        stat->variance = 0.0;
      }
    }
    for (int i = 0; i < n_tests; i++) {
      int set_id = id_set[i];
      if (set_id == -1) continue;
      for (int j = 0; j < n_vars; j++) {
        const struct epd_var *var = &vars[j];
        struct epd_stat *stat = &stats[set_id * n_vars + j];
        double val = var->get(&search_results[i]);
        stat->total += val;
        stat->max = fmax(stat->max, val);
        stat->min = fmin(stat->min, val);
      }
    }
    for (int i = 0; i < n_sets; i++) {
      for (int j = 0; j < n_vars; j++) {
        struct epd_stat *stat = &stats[i * n_vars + j];
        stat->mean = stat->total / (double)epd_sets[i].n_total;
      }
    }
    for (int i = 0; i < n_tests; i++) {
      int set_id = id_set[i];
      if (set_id == -1) continue;
      for (int j = 0; j < n_vars; j++) {
        const struct epd_var *var = &vars[j];
        struct epd_stat *stat = &stats[set_id * n_vars + j];
        double val = var->get(&search_results[i]);
        stat->variance += pow(val - stat->mean, 2.0);
      }
    }
    for (int i = 0; i < n_sets; i++) {
      for (int j = 0; j < n_vars; j++) {
        struct epd_stat *stat = &stats[i * n_vars + j];
        stat->variance /= (double)n_tests;
        stat->std_dev = pow(stat->variance, 0.5);
      }
    }

    printf("  %-30s\n", "Test");
    for (int i = 0; i < n_sets; i++) {
      printf("  %-30s", epd_sets[i].name);
      for (int j = 0; j < n_stats; j++) {
        printf("%-16s ", stat_name[j]);
      }
      printf("\n");
      for (int j = 0; j < n_vars; j++) {
        printf("    %-20s ", vars[j].name);
        double *s = (double *)&stats[i * n_vars + j];
        for (int k = 0; k < n_stats; k++) {
          printf(vars[j].format, s[k]);
        }
        printf("\n");
      }
      printf("Avg. r_check_node %0.2lf\n",
             stats[i * n_vars + 1].total / stats[i * n_vars + 0].total);
      printf("\n");
    }

    free(stats);
  }

  free(search_results);
  free(id_set);

  return 0;
}