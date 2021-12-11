/*
 *  UI commands
 */

#include "engine.h"
#include "fen.h"
#include "log.h"
#include "search.h"
#include "movegen.h"
#include "io.h"
#include "ui.h"

typedef void (*ui_fn)(engine_s *);

/*
 *  Commands
 */ 

void ui_xboard(engine_s *e) { e->xboard_mode = 1; }

/* Game Control */
void ui_quit(engine_s *e) { 
  e->mode = ENGINE_QUIT; 
}
void ui_force(engine_s *e) { 
  e->mode = ENGINE_FORCE_MODE; 
}
void ui_black(engine_s *e) { 
  //e->engine_mode = ENGINE_PLAYING_AS_BLACK;
  //e->game.to_move = WHITE;
}
void ui_white(engine_s *e) { 
  //e->engine_mode = ENGINE_PLAYING_AS_WHITE; 
  //e->game.to_move = BLACK;
}
void ui_go(engine_s *e) { 
  e->mode = e->game.to_move; 
}
void ui_playother(engine_s *e) { 
  e->mode = ENGINE_PLAYING_AS_WHITE + ENGINE_PLAYING_AS_BLACK - e->game.to_move; 
}
void ui_computer(engine_s *e) { }

void ui_result(engine_s *e) {
  PRINT_LOG(&xboard_log, "%s ", get_input());
}
void ui_new(engine_s *e) {
  e->game_n++;
  e->move_n = 1;
  e->resign_delayed = 0;
  e->waiting = 1;
  e->game.to_move = WHITE;
  e->mode = ENGINE_PLAYING_AS_BLACK;
  reset_board(&e->game);
  START_LOG(&think_log, NE_GAME, "g%02d", e->game_n);
  START_LOG(&xboard_log, NE_GAME, "%s", "xboard");
}

/* Engine control */
void ui_sd(engine_s *e) {
  int depth;
  sscanf(get_input(), "%d", &depth);
  /* TODO: depth */
}
void ui_level(engine_s *e) {
  get_input();
  get_input();
  get_input();
}
void ui_otim(engine_s *e) {
  sscanf(get_input(), "%lu", &e->otim);
}

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

/* XBoard accepts an option or feature */
int accept(void)
{
  if(strcmp(get_input(), "accepted") == 0) { 
    return 1;
  }
  return 0;
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

/* Debug */
void ui_print(engine_s *e) {
  print_board(stdout, &(e->game), 0, 0);
}

void ui_attacks(engine_s *e) {
  pos_t target;
  if(parse_pos(get_input(), &target)) {
    return;
  }
  if(no_piece_at_pos(e, target)) {
    return;
  }
  print_board(stdout, &(e->game), target, get_attacks(&(e->game), target, opponent[e->game.to_move])); 
}
  
void ui_moves(engine_s *e) {
  pos_t from;
  if(parse_pos(get_input(), &from)) {
    return;
  }
  if(no_piece_at_pos(e, from)) {
    return;
  }
  print_board(stdout, &(e->game), get_moves(&(e->game), from), pos2mask[from]);
}

void ui_eval(engine_s *e) {
  printf("%d\n", evaluate(&(e->game)));
}

void ui_perft(engine_s *e) {
  int depth;
  if(!sscanf(get_input(), "%d", &depth)) {
    return;
  }
  perft_total(&e->game, depth);
}

void ui_perftd(engine_s *e) {
  int depth;
  if(!sscanf(get_input(), "%d", &depth)) {
    return;
  }
  perft_divide(&e->game, depth);
}

/* No Operation */
void ui_noop(engine_s *e) {
}
void ui_noop_1arg(engine_s *e) {
  get_input();
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
  N_UI_CMDS = 30
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
