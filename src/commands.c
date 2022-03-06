/*
 *  UI commands
 */

#include <stdio.h>

#include "debug.h"
#include "engine.h"
#include "fen.h"
#include "info.h"
#include "io.h"
#include "movegen.h"
#include "options.h"
#include "search.h"
#include "ui.h"

/* --- Commands */

/* -- Game Control */

/* Permanently switch to XBoard mode */
void ui_xboard(engine_s *e) { enter_xboard_mode(e); }

/* Quit the program */
void ui_quit(engine_s *e) { e->mode = ENGINE_QUIT; }

/* "Set the engine to play neither color ("force mode")." */
void ui_force(engine_s *e) { e->mode = ENGINE_FORCE_MODE; }

/* "Leave force mode and set the engine to play the color that is on move."
 * Can also be used in game mode to switch sides and make AI play the current turn */
void ui_go(engine_s *e) { e->mode = e->game.turn; }

/* "Leave force mode and set the engine to play the color that is not on move."
 * Currently not enabled as a feature */
void ui_playother(engine_s *e) {}

/* Not used by CECP v2 */
void ui_black(engine_s *e) {}
void ui_white(engine_s *e) {}

/* Tell this AI that it is playing another AI - not implemented */
void ui_computer(engine_s *e) {}

/* XBoard notifies a result */
void ui_result(engine_s *e) { get_input(); }

/* New game */
void ui_new(engine_s *e) {
  e->game_n++;
  e->move_n = 1;
  e->resign_delayed = 0;
  e->waiting = 1;
  e->game.turn = WHITE;
  e->mode = ENGINE_PLAYING_AS_BLACK;
  reset_board(&e->game);
  history_clear(&e->history);
}

/* -- Engine control */

/* Search depth - not implemented */
void ui_sd(engine_s *e) {
  int depth;
  sscanf(get_input(), "%d", &depth);
  /* TODO: depth */
}

/* Set time control mode - not implemented */
void ui_level(engine_s *e) {
  get_input();
  get_input();
  get_input();
}

/* v1 otim command - not implemented */
void ui_otim(engine_s *e) { sscanf(get_input(), "%lu", &e->otim); }

/* XBoard sets protocol version */
void ui_protover(engine_s *e) {
  int ver;
  sscanf(get_input(), "%d", &ver);
  if (ver > 1) {
    list_features();
    list_options();
  }
}

/* XBoard sets an option value */
void ui_option(engine_s *e) {
  const char *name;
  int reject = 0;

  name = get_delim('=');

  reject = set_option(e, name);
  if (reject == 0) {
    printf("accept\n");
  } else {
    printf("reject\n");
  }
}

/* Handle "accepted" message */
void ui_accepted(engine_s *e) {
  const char *arg;
  arg = get_input();
  if (strcmp(arg, "option") == 0) {
  } else {
    feature_accepted(arg);
  }
}

/* -- FEN input and output */

/* Set game state from FEN input */
void ui_fen(engine_s *e) {
  char placement[100];
  get_input_to_buf(placement, sizeof(placement));
  char active[100];
  get_input_to_buf(active, sizeof(active));
  char castling[100];
  get_input_to_buf(castling, sizeof(castling));
  char enpassant[100];
  get_input_to_buf(enpassant, sizeof(enpassant));
  char halfmove[100];
  get_input_to_buf(halfmove, sizeof(halfmove));
  char fullmove[100];
  get_input_to_buf(fullmove, sizeof(fullmove));
  if (load_fen(&e->game, placement, active, castling, enpassant, halfmove, fullmove)) {
    printf("sFEN string not recognised\n");
  }
}

/* Output game state */
void ui_getfen(engine_s *e) {
  char buf[100];
  get_fen(&e->game, buf, sizeof(buf));
  printf("%s\n", buf);
}

/* -- Debugging */

/* Print board */
void ui_print(engine_s *e) { print_board(&(e->game), 0, 0); }

/* Print board showing pieces attacking a target square */
void ui_attacks(engine_s *e) {
  square_e target;
  if (parse_square(get_input(), &target)) {
    return;
  }
  if (ui_no_piece_at_square(e, target)) {
    return;
  }
  print_board(&(e->game), target, get_attacks(&(e->game), target, opponent[e->game.turn]));
}

/* Print board showing squares a piece can move to */
void ui_moves(engine_s *e) {
  square_e from;
  if (parse_square(get_input(), &from)) {
    return;
  }
  if (ui_no_piece_at_square(e, from)) {
    return;
  }
  print_board(&(e->game), get_moves(&(e->game), from), square2bit[from]);
}

/* Evaluate the position and print the score */
void ui_eval(engine_s *e) { printf("%d\n", evaluate(&(e->game))); }

/* Run perft to a specified depth */
void ui_perft(engine_s *e) {
  int depth;
  if (!sscanf(get_input(), "%d", &depth)) {
    return;
  }
  perft_total(&e->game, depth);
}

/* Run perft-divide */
void ui_perftd(engine_s *e) {
  int depth;
  if (!sscanf(get_input(), "%d", &depth)) {
    return;
  }
  perft_divide(&e->game, depth);
}

/* Print program info */
void ui_info(engine_s *e) { print_program_info(); }

/* No Operation */
void ui_noop(engine_s *e) {}
void ui_noop_1arg(engine_s *e) { get_input(); }

/*
 *  Command Table
 */

/* Type of command - help display sorts commands by these types */
typedef enum ui_cmd_type_e { CT_GAMECTL, CT_DISPLAY, CT_XBOARD, CT_UNIMP } ui_cmd_type_e;

/* Entry in the table of commands */
typedef struct ui_cmd_s_ {
  ui_cmd_type_e type;
  char cmd[10];
  ui_fn fn;
  char desc[100];
} ui_cmd_s;

void ui_help(engine_s *e);

/* Table of commands */
const ui_cmd_s cmds[] = {
    /* clang-format off */
  { CT_DISPLAY, "attacks",  ui_attacks,    "POS  - Display all pieces that can attack POS" },
  { CT_XBOARD,  "accepted", ui_accepted,   "     - ???" },
  { CT_GAMECTL, "black",    ui_black,      "     - AI to play as black" },
  { CT_XBOARD,  "computer", ui_computer,   "     - ???" },
  { CT_DISPLAY, "eval",     ui_eval,       "     - Evaluate game" },
  { CT_GAMECTL, "fen",      ui_fen,        "FEN  - Set the position using a FEN string" },
  { CT_GAMECTL, "force",    ui_force,      "     - Enter force mode" },
  { CT_GAMECTL, "getfen",   ui_getfen,     "     - Get the position in FEN notation" },
  { CT_GAMECTL, "go",       ui_go,         "     - AI to make first move if playing as white" },
  { CT_GAMECTL, "help",     ui_help,       "     - Display a list of all commands" },
  { CT_DISPLAY, "info",     ui_info,       "     - Display build information"},
  { CT_XBOARD,  "level",    ui_level,      "     - ???"},
  { CT_DISPLAY, "moves",    ui_moves,      "POS  - Display all squares that the piece at POS can move to" },
  { CT_GAMECTL, "new",      ui_new,        "     - New game" },
  { CT_XBOARD,  "option",   ui_option,     "     - Set engine option" },
  { CT_UNIMP,   "otim",     ui_noop_1arg,  "TIME - This function is accepted but currently has no effect" },
  { CT_GAMECTL, "perft",    ui_perft,      "     - Move generator performance test" },
  { CT_GAMECTL, "perftd",   ui_perftd,     "     - Move generator performance test, divided by move" },
  { CT_DISPLAY, "print",    ui_print,      "     - Display the board" },
  { CT_XBOARD,  "protover", ui_protover,   "PROT - ??? Selects an XBoard protocol of at least PROT" },
  { CT_GAMECTL, "quit",     ui_quit,       "     - Quit the program" },
  { CT_GAMECTL, "q",        ui_quit,       "     - Quit the program more quickly" },
  { CT_UNIMP,   "st",       ui_noop_1arg,  "     - This function is accepted but currently has no effect" },
  { CT_GAMECTL, "sd",       ui_sd,         "D    - Set the search depth" },
  { CT_UNIMP,   "time",     ui_noop_1arg,  "     - This function is accepted but currently has no effect" },
  { CT_UNIMP,   "random",   ui_noop,       "     - This function is accepted but currently has no effect" },
  { CT_UNIMP,   "result",   ui_result,     "     - This function is accepted but currently has no effect" },
  { CT_UNIMP,   "undo",     ui_noop,       "     - This function is accepted but currently has no effect" },
  { CT_UNIMP,   "variant",  ui_noop_1arg,  "     - This function is accepted but currently has no effect" },
  { CT_GAMECTL, "white",    ui_white,      "     - AI to play as white (enter 'go' to start)" },
  { CT_GAMECTL, "xboard",   ui_xboard,     "     - Enter XBoard mode" }
    /* clang-format on */
};

/* Number of commands */
enum { N_UI_CMDS = sizeof(cmds) / sizeof(*cmds) };

/* Help command - display a list of all commands, grouped by type */
void ui_help(engine_s *e) {
  printf("\n  Command List\n");
  printf("\n   The following commands are used for starting and controlling games:\n");
  for (int i = 0; i < N_UI_CMDS; i++) {
    if (cmds[i].type == CT_GAMECTL) {
      printf("    %-10s%s\n", cmds[i].cmd, cmds[i].desc);
    }
  }
  printf("\n   The following commands are used to display the state of the game:\n");
  for (int i = 0; i < N_UI_CMDS; i++) {
    if (cmds[i].type == CT_DISPLAY) {
      printf("    %-10s%s\n", cmds[i].cmd, cmds[i].desc);
    }
  }
  printf("\n   The following commands are used by XBoard:\n");
  for (int i = 0; i < N_UI_CMDS; i++) {
    if (cmds[i].type == CT_XBOARD) {
      printf("    %-10s%s\n", cmds[i].cmd, cmds[i].desc);
    }
  }
  printf(
      "\n   The following commands are accepted for compatibility with XBoard, but have no "
      "effect:");
  int j = 0;
  for (int i = 0; i < N_UI_CMDS; i++) {
    if (cmds[i].type == CT_UNIMP) {
      if ((j % 6) == 0) {
        printf("\n    ");
      }
      printf("%-10s", cmds[i].cmd);
      j++;
    }
  }
  printf("\n");
}

int accept_command(engine_s *e, const char *in) {
  int i;
  /* Search commands and call function if found */
  for (i = 0; i < N_UI_CMDS; i++) {
    if (strcmp(in, cmds[i].cmd) == 0) {
      (*cmds[i].fn)(e);
      /* Success */
      return 0;
    }
  }
  /* Command not found */
  return 1;
}
