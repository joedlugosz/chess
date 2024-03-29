/*
 *  Entry point to the chess engine app
 *  Builds executable chess
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "engine.h"
#include "evaluate.h"
#include "hash.h"
#include "os.h"
#include "ui.h"

void parse_command_line_args(struct engine *e, int argc, char *argv[]) {
  for (int arg = 1; arg < argc; arg++) {
    if (strcmp(argv[arg], "x") == 0) {
      enter_xboard_mode(e);
    }
    if (strcmp(argv[arg], "t") == 0) {
      /* For testing - like XBoard mode but with Ctrl-C */
      e->xboard_mode = 1;
    }
  }
}

int main(int argc, char *argv[]) {
  debug_init();
  setbuf(stdout, NULL);
  setup_signal_handlers();
  init_board();
  evaluate_init();
  hash_init();
  tt_init();

  /* Genuine(ish) random numbers are used where repeatability is not desirable */
  srand(clock());

  struct engine engine;
  init_engine(&engine);
  parse_command_line_args(&engine, argc, argv);
  run_engine(&engine);

  tt_exit();

  return 0;
}
