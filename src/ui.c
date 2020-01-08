/*
 *  ui.c
 *
 *  Program entry point and user interface.
 *
 *  Features:
 *   * Console game mode
 *   * XBoard mode supporting most of v2 protocol
 *   * Extra debugging commands
 *
 *  Limitations:
 *   * SIGINT and SIGTERM are ignored
 *
 *  Revisions:
 *   0.1  09/04/16  Current
 *   4.1  18/02/18  Minor changes due to Windows support
 *   4.2  19/02/18  Refactoring
 *   5.0  17/02/19  Simplified file structure
 */

//#include "language.h"
#include "chess.h"
#include "search.h"
#include "sys.h"
#include "os.h"
#include "log.h"

#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>


enum {
  ERR_MOVING   = 0,
  ERR_NOPIECE  = ERR_MOVING + 0,
  ERR_CANTMOVETOSQUARE = ERR_MOVING + 1,
  N_ERR 
};

const char err_msg[N_ERR][80] = {
};

//engine_s engine;

/* XBoard comms log */
log_s xboard_log = {
  .new_every = NE_SESSION
};
/* Fatal error log */
log_s error_log = {
  .new_every = NE_SESSION
};

clock_t t1, t2;

/*
 *  Options
 */ 

enum {
  N_UI_OPTS = 1
};

const option_s _ui_opts[N_UI_OPTS] = {
  { "New XBoard log",   COMBO_OPT, &(xboard_log.new_every), 0, 0, &newevery_combo },
};
const options_s ui_opts = { N_UI_OPTS, _ui_opts };

/*
 *  Logging
 */
void start_session_log(void)
{
  START_LOG(&think_log, NE_SESSION, "%s", "");
  START_LOG(&xboard_log, NE_SESSION, "%s", "xboard");
  START_LOG(&error_log, NE_SESSION, "%s", "error");
}
void start_move_log(engine_s *e)
{
#if (LOGGING == YES)
  char buf[20];
  snprintf(buf, sizeof(buf), "g%02d-%c%02d", 
    e->game_n, (e->engine_mode == ENGINE_PLAYING_AS_WHITE) ? 'w' : 'b', e->move_n);
  START_LOG(&think_log, NE_MOVE, "%s", buf);
  START_LOG(&xboard_log, NE_MOVE, "%s-xboard", buf);
#endif
}


/*
 *  Output
 */
static inline void print_statistics(engine_s *engine) {
  if(!engine->xboard_mode) {
    double time = (double)(t2 - t1) / (double)CLOCKS_PER_SEC;
    printf("%d : %0.2lf sec", 
	   eval(&engine->game)/10, time);
    if(engine->game.to_move == engine->engine_mode) {
      printf(" : %d nodes : %0.1lf%% cutoff %0.2lf Knps", engine->result.n_searched,
	     engine->result.cutoff, (double)engine->result.n_searched / (time * 1000.0));
    }
    printf("\n");
  }
}

static inline void print_game_state(engine_s *engine)
{
  if(!engine->xboard_mode) {
    print_board(stdout, &engine->game, 0, 0);
    if(in_check(&engine->game)) {
      printf("%s is in check.\n", player_text[engine->game.to_move]);
    }
  }
}

static inline void print_prompt(engine_s *engine)
{
  if(!engine->xboard_mode) {
    if(engine->engine_mode != FORCE_MODE) {
      printf("%s %s> ", player_text[engine->game.to_move],
	     engine->game.to_move == engine->engine_mode ? "(AI) " : "");
    } else {
      printf("FORCE > ");
    }
  }
}

static inline void print_ai_resign(engine_s *engine)
{
  if(engine->xboard_mode) {
    printf("resign\n");
  } else {
    printf("%s resigns.", player_text[engine->game.to_move]);
  }
}

static inline void print_ai_move(engine_s *engine, char *buf)
{
  if(engine->xboard_mode) {
    printf("move %s\n", buf);
  } else {
    print_prompt(engine);
    printf("%s\n", buf);
  }
}

static void print_msg(engine_s *engine, const char *fmt, pos_t from, pos_t to) {
  if(!engine->xboard_mode) {
    if(from >= 0) {
      char from_buf[10];
      encode_position(from_buf, from);
      if(to >= 0) {
	char to_buf[10];
	encode_position(to_buf, to);
	printf(fmt, from_buf, to_buf);
      } else {
	printf(fmt, from_buf);
      }
    } else {
      printf("%s", fmt);
    }
  }
}

/* 
 *  Move Checking 
 */
int check_force_move(engine_s *engine, pos_t from)
{
  if((pos2mask[from] & engine->game.total_a) == 0) {
    print_msg(engine, "There is no piece at %s.\n", from, -1);
    return 1;
  } 
  return 0;
}

int check_move(engine_s *engine, pos_t from, pos_t to)
{
  if(check_force_move(engine, from)) {
    return 1;
  }
  if(from == to) {
    print_msg(engine, "The origin %s is the same as the destination.\n", from, -1);
    return 1;
  } 
  if((pos2mask[from] & get_my_pieces(&engine->game)) == 0) {
    print_msg(engine, "The piece at %s is not your piece.\n", from, -1);
    return 1;
  } 
  if((pos2mask[to] & get_moves(&engine->game, from)) == 0) { 
    print_msg(engine, "The piece at %s cannot move to %s.\n", from, to);
    return 1;
  } 
  return 0;
}


void init_engine(engine_s *engine)
{
  /* Set up the chess board */
  reset_board(&engine->game);
  /* Game control initial values */
  engine->xboard_mode = 0;
  engine->resign = 0;
  engine->game_n = 1;
  engine->waiting = 1;
  engine->engine_mode = ENGINE_PLAYING_AS_BLACK;
}

void finished_move(engine_s *engine)
{
  /* It is now the other players turn */
  change_player(&engine->game);
  /* move_n holds number of complete moves, incremented when it is
     white's move */
  if(engine->game.to_move == WHITE) {
    engine->move_n++;
  }    
}

static inline int ai_to_move(engine_s *engine)
{
  if(engine->game.to_move != engine->engine_mode) return 0;
  else return 1;
}

static inline void ai_move(engine_s *engine)
{
  /* Start move log */
  start_move_log(engine);
  /* Do AI move */
  t1 = clock();
    do_ai_move(&engine->game, &engine->result);
  t2 = clock();
  /* Resign if appropriate */
  if(engine->resign || engine->result.status == CHECKMATE) {
    PRINT_LOG(&xboard_log, "%s", "\nAI > resign");
    print_ai_resign(engine);
    engine->engine_mode = FORCE_MODE;
    return;
  }
  /* If no valid move was found */
  /* Delay resign by a cycle to see if checkmate is given */
  if(engine->result.status == STALEMATE) {
    engine->resign = 1;
    return;
  }
  char buf[20];
  ASSERT(engine->engine_mode < FORCE_MODE);
  encode_move(buf, engine->game.from, engine->game.to, engine->game.captured,
	      engine->game.check[opponent[engine->engine_mode]]);
  /* Log move */
  PRINT_LOG(&xboard_log, "\nAI > %s", buf);
  /* Output */
  print_ai_move(engine, buf);
  print_statistics(engine);
  print_game_state(engine);
  /* Completed a valid move */
  finished_move(engine);
  /* Start timer for user move */
  t1 = clock();
}

/* Accept a valid user move.
   Return:  0 - Valid move
            1 - Well formed but not valid
	    2 - Not recognised */
static inline int accept_move(engine_s *engine, const char *in)
{
  pos_t from, to;
  
  /* If not a valid chess instruction */
  if(decode_instruction(in, &from, &to)) {
    return 2;
  }
  /* Normal mode */
  if(engine->engine_mode != FORCE_MODE) {
    /* Check from and to */
    if(check_move(engine, from, to)) {
      /* Invalid move */
      return 1;
    } else {
      /* If a valid move has been entered, mark the time taken */
      t2 = clock();
      /* Don't count elapsed time while waiting for first move */
      if(engine->waiting) { 
	  t1 = t2;
      } 	  
      /* If waiting after a new game, first user move begins the game as white */
      engine->waiting = 0;
    }
  } else {
    /* Force mode - Only check that there is a piece to move */
    if(check_force_move(engine, from)) {
      /* Invalid move */
      return 1;
    }
  }    
  /* Update the game state */
  /* TODO: promotion */
  do_move(&engine->game, from, to, 0);
  /* Success */
  return 0;
}

static inline void user_input(engine_s *engine)
{
  const char *in;
  in = get_input();
  /* No input */
  if(in[0] == 0) {
    return;
  }
  /* Message from server etc. */
  if(in[0] == '{') {
    PRINT_LOG(&xboard_log, "Msg : %s", in+1);
    return;
  }
  /* Command */
  if(!accept_command(engine, in)) {
    return;
  }
  /* Move */
  if(!accept_move(engine, in)) {
    if(engine->engine_mode < FORCE_MODE) {
      print_statistics(engine);
    }
    print_game_state(engine);
    finished_move(engine);
    return;
  }
  /* Not a command or a move */
  print_msg(engine, "Unrecognised command\nEnter 'help' for a list of commands.\n", -1, -1);
}

/*
 *  UI Main Loop
 */
void main_loop(engine_s *engine) 
{
  while(engine->engine_mode != QUIT) {
    if(!ai_to_move(engine)) {
      print_prompt(engine);
    }
    if(ai_to_move(engine)) {
      ai_move(engine);
    } else {
      user_input(engine);
    }
  }
}

void parse_command_line_args(engine_s *e, int argc, char *argv[])
{
  for(int arg = 1; arg < argc; arg++) {
    if(strcmp(argv[arg], "x") == 0) {
      e->xboard_mode = 1;
    }
  }
}

int main(int argc, char *argv[])
{
  start_session_log();
  setbuf(stdout, NULL);
  init_os();
  init_board();
  srand(clock());

  engine_s engine;
  init_engine(&engine);
  parse_command_line_args(&engine, argc, argv);
  print_game_state(&engine);
  main_loop(&engine);
}
