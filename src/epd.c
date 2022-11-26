/*
 * Functions which run Extended Position Description (EPD) test cases
 *
 * EPD specifies a test case as a line of text containing a position to be
 * searched in PGN, with multiple commands. Some commands have actions which are
 * executed before the search, some have actions after the search, and some have
 * both. Currently, only a few commands are implemented. Commands return a PASS
 * or FAIL result, and an overall PASS/FAIL is recorded for each case.
 *
 * Cases can be identified and organised into sets by the "id" command which specifies
 * the name of the set and the number of the case within the set, e.g. `id "BK.01";`
 * Results are organised by set.
 */
#include <ctype.h>
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
int show_board = 0;
int show_thought = 0;

/* Target value for "dm" moves */
int direct_mate = 0;
/* Number of fullmoves to mate */
int full_move = 0;

/* A set of EPD cases */
struct epd_set {
  char name[MAX_SET_NAME];
  int n_pass;
  int n_total;
};

/* List of all EPD sets */
struct epd_set epd_sets[MAX_SET];

/* The total number of EPD sets recorded so far */
int n_sets;

/* The set of the current command */
char current_set[MAX_SET_NAME];

/* Add a new set of EPD cases to the global list */
struct epd_set *add_set(const char *name) {
  strcpy(epd_sets[n_sets].name, name);
  return &epd_sets[n_sets++];
}

/* Find an existing set of EPD cases in the global list */
struct epd_set *find_set(const char *name) {
  for (int i = 0; i < n_sets; i++) {
    if (strcmp(epd_sets[i].name, name) == 0) return &epd_sets[i];
  }
  return 0;
}

/* Add an EPD test result for a test within a set */
static void add_result(const char *name, int pass) {
  struct epd_set *set = find_set(name);
  if (!set) set = add_set(name);
  if (set) {
    if (pass) set->n_pass++;
    set->n_total++;
  }
}

/* Print the total pass/fail results by set */
static void print_results(void) {
  for (int i = 0; i < n_sets; i++) {
    printf("   %-60s %d/%d %0.2lf%%\n", epd_sets[i].name, epd_sets[i].n_pass, epd_sets[i].n_total,
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
int epd_run(char *epd_line, int depth) {
  struct position position;
  int pass = 1;
  direct_mate = 0;

  char *placement_text = strtok(epd_line, " ");
  char *active_player_text = strtok(0, " ");
  char *castling_text = strtok(0, " ");
  char *en_passant_text = strtok(0, " ");

  load_fen(&position, placement_text, active_player_text, castling_text, en_passant_text, "0", "1");

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
    search(depth, &history, &position, &result, show_board);
    memcpy(&first_result, &result, sizeof(first_result));
  } else {
    while (full_move <= direct_mate) {
      search(depth, &history, &position, &result, show_board);
      if (full_move == 0) memcpy(&first_result, &result, sizeof(first_result));
      if (result.move.result & MATE) break;
      make_move(&position, &result.move);
      change_player(&position);
      search(depth, &history, &position, &result, show_thought);
      if (result.move.result & MATE) break;
      make_move(&position, &result.move);
      change_player(&position);
      full_move++;
    }
  }

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

  int index = 0;
  int n_pass = 0;
  while (fgets(epd_line, sizeof(epd_line), f)) {
    if (*epd_line == '#') continue;
    int pass = epd_run(epd_line, depth);
    index++;
    char buf[2000];
    sprintf(buf, "%d/%d %s", index, n_tests, output);

    if (!pass) n_pass++;

    if (pass || show_pass) {
      printf("%s\n", buf);
    }

    /* current_set is set by epd_id */
    if (current_set[0]) add_result(current_set, pass);
  }

  fclose(f);

  if (show_total) {
    printf("\nResults by category:\n");
    print_results();
    printf("\nTotal:\n");
    printf("   %d/%d %0.2lf%%\n", n_pass, n_tests, (double)n_pass / (double)n_tests);
  }

  return 0;
}