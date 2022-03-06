#include <ctype.h>
#include <stdio.h>

#include "fen.h"
#include "history.h"
#include "search.h"
#include "state.h"

char epd_cmd[100][1000];
char *epd_arg[100];
char id[1000];
char output[1100];

int show_pass = 1;
int show_fail = 1;
int show_total = 1;
int show_board = 0;
int show_thought = 0;

int direct_mate = 0;
int full_move = 0;

enum { MAX_SET = 100, MAX_SET_NAME = 100 };

struct epd_set {
  char name[MAX_SET_NAME];
  int n_pass;
  int n_total;
};

struct epd_set epd_sets[MAX_SET];
int n_sets;
char current_set[MAX_SET_NAME];

struct epd_set *add_set(const char *name) {
  strcpy(epd_sets[n_sets].name, name);
  return &epd_sets[n_sets++];
}

struct epd_set *find_set(const char *name) {
  for (int i = 0; i < n_sets; i++) {
    if (strcmp(epd_sets[i].name, name) == 0) return &epd_sets[i];
  }
  return 0;
}

void add_result(const char *name, int pass) {
  struct epd_set *set = find_set(name);
  if (!set) set = add_set(name);
  if (set) {
    if (pass) set->n_pass++;
    set->n_total++;
  }
}

void print_results(void) {
  for (int i = 0; i < n_sets; i++) {
    printf("   %-60s %d/%d %0.2lf%%\n", epd_sets[i].name, epd_sets[i].n_pass, epd_sets[i].n_total,
           (double)epd_sets[i].n_pass / (double)epd_sets[i].n_total * 100.0);
  }
}

typedef int (*epd_fn)(char *, search_result_s *);

int epd_bm(char *args, search_result_s *result) {
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

int epd_dm_pre(char *args, search_result_s *result) {
  if (sscanf(args, "%d", &direct_mate) != 1) return 1;
  return 0;
}

int epd_dm_post(char *args, search_result_s *result) { return (full_move > direct_mate) ? 1 : 0; }

int epd_id(char *args, search_result_s *result) {
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

int epd_comment(char *args, search_result_s *result) { return 0; }

struct epd_cmd {
  char name[100];
  epd_fn fn;
};

struct epd_cmd epd_cmds_post[] = {{"bm", epd_bm}, {"dm", epd_dm_post}, {"#", epd_comment}, {"", 0}};

struct epd_cmd epd_cmds_pre[] = {{"dm", epd_dm_pre}, {"id", epd_id}, {"", 0}};

epd_fn epd_find_cmd(const char *name, struct epd_cmd *epd_cmds) {
  struct epd_cmd *cmd = epd_cmds;

  while (cmd->name[0]) {
    if (strcmp(cmd->name, name) == 0) return (cmd->fn);
    cmd++;
  }

  /* Not found */
  return 0;
}

int epd_run(char *epd_line, int depth) {
  state_s position;
  int pass = 1;
  direct_mate = 0;

  char *placement_text = strtok(epd_line, " ");
  char *active_player_text = strtok(0, " ");
  char *castling_text = strtok(0, " ");
  char *en_passant_text = strtok(0, " ");

  load_fen(&position, placement_text, active_player_text, castling_text, en_passant_text, "0", "1");

  if (show_board) print_board(&position, 0, 0);

  /* Parse remainder of line into list of EPD operations, delimited by ';' */
  char *op;
  int n_ops = 0;
  while ((op = strtok(0, ";"))) {
    if (!op || !*op) break;
    while (*op == ' ') op++;
    if (*op == '#') break;
    strcpy(&epd_cmd[n_ops][0], op);
    n_ops++;
  }

  /* Parse each EPD operation into command and arguments, delimited by ' ' */
  for (int i = 0; i < n_ops; i++) {
    strtok(epd_cmd[i], " ");
    epd_arg[i] = strtok(0, "");
  }

  output[0] = 0;
  current_set[0] = 0;

  /* Match operations that should be executed before search */
  for (int i = 0; i < n_ops; i++) {
    char *name = epd_cmd[i];
    if (!epd_cmd[i][0]) break;
    epd_fn fn;
    if ((fn = epd_find_cmd(name, epd_cmds_pre))) (*fn)(epd_arg[i], 0);
  }

  struct history history;
  memset(&history, 0, sizeof(history));
  full_move = 0;

  search_result_s first_result;
  search_result_s result;
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

  /* Match operations that should be executed after search */
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
        strcat(output, "FAIL; ");
        pass = 0;
      } else {
        strcat(output, "PASS; ");
      }
    }
  }
  return pass;
}

/* Run tests from a file of EPD positions. Each line of the file contains
   and EPD phrase */
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