/*
 *   4.1 18/02/2018 Added 'help' command
 *   5.0 24/02/2019 Added 'perft' command
 */
#include "chess.h"
#include "sys.h"
#include "log.h"
#include "search.h"
#include "movegen.h"

/*
 *  Commands
 */ 

/* Handles "protover" command - for protocol version >= 2 */
void ui_protover(engine_s *e) {
  int ver;
  sscanf(get_input(), "%d", &ver);
  PRINT_LOG(&xboard_log, "%d", ver);
  if(ver > 1) {
    list_features();
    list_options();
  }
}

/* Handles "option" command - XBoard sets an option value */
void ui_option(engine_s *e)
{
  const char *name;
  int reject = 0;

  name = get_delim('=');
  PRINT_LOG(&xboard_log, "%s=", name);

  reject = set_option(e, name);
  if(reject == 0) {
    printf("accept\n");
    PRINT_LOG(&xboard_log, "%s", "\naccept");
  } else {
    printf("reject\n");
    PRINT_LOG(&xboard_log, "%s", "\nreject");
  }
}

/* Handles "accepted" message */
void ui_accepted(engine_s *e) {
  const char *arg;
  arg = get_input();
  if(strcmp(arg, "option") == 0) {
  } else {
    feature_accepted(arg);
  }
}

/* Handles "xboard" command - Switch to XBoard mode */
void ui_xboard(engine_s *e) {
  e->xboard_mode = 1;
}

/* Engine control */
void ui_sd(engine_s *e) {
  int depth;
  sscanf(get_input(), "%d", &depth);
  set_depth(depth);
}
void ui_level(engine_s *e) {
  get_input();
  get_input();
  get_input();
}
void ui_otim(engine_s *e) {
  sscanf(get_input(), "%lu", &e->otim);
}

/* Game Control */
void ui_black(engine_s *e) {
  e->ai_player = BLACK;
  e->force = 0;
}
void ui_white(engine_s *e) {
  e->ai_player = WHITE;
  e->force = 0;
}
void ui_new(engine_s *e) {
  e->game_n++;
  e->move_n = 1;
  e->resign = 0;
  e->waiting = 1;
  e->game.to_move = WHITE;
  e->ai_player = BLACK;
  reset_board(&e->game);
  START_LOG(&think_log, NE_GAME, "g%02d", e->game_n);
  START_LOG(&xboard_log, NE_GAME, "%s", "xboard");
}
void ui_fen(engine_s *e) {
  const char *placement = get_input();
  const char *active = get_input();
  const char *castling = get_input();
  const char *enpassant = get_input();
  if(load_fen(&e->game, placement, active, castling, enpassant)) {
    printf("FEN string not recognised\n");
  }
}
void ui_getfen(engine_s *e) {
  char buf[100];
  get_fen(&e->game, buf, sizeof(buf));
  printf("%s\n", buf);
}
void ui_quit(engine_s *e) {
  e->run = 0;
}
void ui_result(engine_s *e) {
  PRINT_LOG(&xboard_log, "%s ", get_input());
}
void ui_force(engine_s *e) {
  e->force = 1;
}
void ui_go(engine_s *e) {
  e->waiting = 0;
}
void ui_computer(engine_s *e) {
}

/* No Operation */
void ui_noop(engine_s *e) {
}
void ui_noop_1arg(engine_s *e) {
  get_input();
}

/* XBoard accepts an option or feature */
int accept(void)
{
  if(strcmp(get_input(), "accepted") == 0) { 
    return 1;
  }
  return 0;
}

/* Debug */
void ui_print(engine_s *e) {
  print_board(stdout, &(e->game), 0, 0);
}

void ui_attacks(engine_s *e) {
  pos_t target;
  if(decode_position(get_input(), &target)) {
    return;
  }
  if(check_force_move(&(e->game), target)) {
    printf("No piece\n");
    return;
  }
  print_board(stdout, &(e->game), target, get_attacks(&(e->game), target, opponent[e->game.to_move])); 
}
  
void ui_moves(engine_s *e) {
  pos_t from;
  if(decode_position(get_input(), &from)) {
    return;
  }
  if(check_force_move(&(e->game), from)) {
    printf("No piece\n");
    return;
  }
  print_board(stdout, &(e->game), get_moves(&(e->game), from), pos2mask[from]);
}

void ui_eval(engine_s *e) {
  printf("%d\n", eval(&(e->game)));
}

void ui_perft(engine_s *e) {
  int depth, i;
  sscanf(get_input(), "%d", &depth);
  printf("%8s%16s%12s%12s%12s%12s%12s%12s%12s%12s\n", "Depth", "Nodes", 
    "Captures", "E.P.", "Castles", "Promotions", "Checks", "Disco Chx", 
    "Double Chx", "Checkmates");
  for(i = 0; i < depth; i++) {
    perft_s data;
    perft(&data, &(e->game), i);
    printf("%8d%16lld%12ld%12s%12ld%12s%12ld%12s%12s%12ld\n", 
      i, data.moves, data.captures, "X", data.castles, "X", data.checks, 
      "X", "X", data.checkmates);
  }
  //encode_move(buf, move->from, move->to, next_state.captured, 0);
  //printf("%4d %20s %12lld\n", depth, buf, p);
  //printf("Perft: %lld\n", perft(&(e->game), depth));
}

/*
 *  Command Table
 */

typedef enum ui_cmd_type_e {
  CT_GAMECTL,
  CT_DISPLAY,
  CT_XBOARD,
  CT_UNIMP
} ui_cmd_type_e;
typedef struct ui_cmd_s_ {
  ui_cmd_type_e type;
  char cmd[10];
  ui_fn fn;
  char desc[100];
} ui_cmd_s;

enum {
  N_UI_CMDS = 29
};

const ui_cmd_s cmds[N_UI_CMDS];
void ui_help(engine_s *e) {
  printf("\n  Command List\n");
  printf("\n   The following commands are used for starting and controlling games:\n");
  for(int i = 0; i < N_UI_CMDS; i++) {
    if(cmds[i].type == CT_GAMECTL) {
      printf("    %-10s%s\n", cmds[i].cmd, cmds[i].desc);
    }
  }
  printf("\n   The following commands are used to display the state of the game:\n");
  for(int i = 0; i < N_UI_CMDS; i++) {
    if(cmds[i].type == CT_DISPLAY) {
      printf("    %-10s%s\n", cmds[i].cmd, cmds[i].desc);
    }
  }
  printf("\n   The following commands are used by XBoard:\n");
  for(int i = 0; i < N_UI_CMDS; i++) {
    if(cmds[i].type == CT_XBOARD) {
      printf("    %-10s%s\n", cmds[i].cmd, cmds[i].desc);
    }
  }
  printf("\n   The following commands are accepted for compatibility with XBoard, but have no effect:");
  int j = 0;
  for(int i = 0; i < N_UI_CMDS; i++) {
    if(cmds[i].type == CT_UNIMP) {
      if((j % 6) == 0) {
	printf("\n    ");
      }
      printf("%-10s", cmds[i].cmd);
      j++;
    }
  }
  printf("\n");
}

const ui_cmd_s cmds[N_UI_CMDS] = {
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
  { CT_XBOARD,  "level",    ui_level,      "     - ???"},
  { CT_DISPLAY, "moves",    ui_moves,      "POS  - Display all squares that the piece at POS can move to" },
  { CT_GAMECTL, "new",      ui_new,        "     - New game" },
  { CT_XBOARD,  "option",   ui_option,     "     - Set engine option" },
  { CT_UNIMP,   "otim",     ui_noop_1arg,  "TIME - This function is accepted but currently has no effect" },
  { CT_GAMECTL, "perft",    ui_perft,      "     - Move generator performance test" },
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
};

int accept_command(engine_s *e, const char *in)
{
  int i;
  /* Search commands and call function if found */
  for(i = 0; i < N_UI_CMDS; i++) {
    if(strcmp(in, cmds[i].cmd) == 0) {
      (*cmds[i].fn)(e);
      PRINT_LOG(&xboard_log, "Cmd %s", in);
      /* Success */
      return 0;
    }
  }
  /* Command not found */
  return 1;
}
