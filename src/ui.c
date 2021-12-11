/*
 *  Program entry point and user interface.
 */

#include "commands.h"
#include "engine.h"
#include "log.h"
#include "os.h"
#include "search.h"
#include "io.h"

#include <time.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

enum { POS_BUF_SIZE = 3, 
       MOVE_BUF_SIZE = 10 };

log_s xboard_log = { .new_every = NE_SESSION };
log_s error_log = { .new_every = NE_SESSION };

static const option_s options[] = {
  { "New XBoard log",   COMBO_OPT, &(xboard_log.new_every), 0, 0, &newevery_combo },
};
const options_s engine_options = { 
  sizeof(options)/sizeof(options[0]), options 
};

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
  char buf[100];
  snprintf(buf, sizeof(buf), "g%02d-%c%02d", 
    e->game_n, (e->mode == ENGINE_PLAYING_AS_WHITE) ? 'w' : 'b', e->move_n);
  START_LOG(&think_log, NE_MOVE, "%s", buf);
  START_LOG(&xboard_log, NE_MOVE, "%s-xboard", buf);
#endif
}

/*
 *  Clocks
 */
static inline void reset_time(engine_s *engine)
{
  engine->start_time = clock();
  engine->elapsed_time = 0;
}
static inline void mark_time(engine_s *engine)
{
  engine->elapsed_time = clock() - engine->start_time;
}
static inline clock_t get_time(engine_s *engine) 
{
  return engine->elapsed_time;
}

static inline int ai_turn(engine_s *engine)
{
  if(engine->game.turn != engine->mode) return 0;
  else return 1;
}

static inline int is_in_normal_play(engine_s *engine) {
  if(engine->mode >= ENGINE_FORCE_MODE) return 0;
  else return 1;
}


/*
 *  Output
 */
static inline void print_statistics(engine_s *engine, search_result_s *result) {
  if(!engine->xboard_mode) {
    double time = (double)(get_time(engine)) / (double)CLOCKS_PER_SEC;
    printf("\n\n%d : %0.2lf sec", evaluate(&engine->game)/10, time);
    if(ai_turn(engine) && result) {
      printf(" : %d nodes : %0.1lf%% cutoff %0.2lfk node/s", result->n_searched,
	      result->cutoff, (double)result->n_searched / (time * 1000.0));
    }
    printf("\n");
  }
}

static inline void print_game_state(engine_s *engine)
{
  if(!engine->xboard_mode) {
    print_board(stdout, &engine->game, 0, 0);
    if(in_check(&engine->game)) {
      printf("%s is in check.\n", player_text[engine->game.turn]);
    }
  }
}

static inline void print_prompt(engine_s *engine)
{
  if(!engine->xboard_mode) {
    if(engine->mode != ENGINE_FORCE_MODE) {
      printf("%s %s> ", player_text[engine->game.turn],
	     engine->game.turn == engine->mode ? "(AI) " : "");
    } else {
      printf("FORCE > ");
    }
  }
}

static inline void print_ai_resign(engine_s *engine)
{
  PRINT_LOG(&xboard_log, "%s", "\nAI > resign");
  if(engine->xboard_mode) {
    printf("resign\n");
  } else {
    printf("%s resigns.", player_text[engine->game.turn]);
  }
}

static inline void print_ai_move(engine_s *engine, search_result_s *result)
{
  ASSERT(is_in_normal_play(engine));

  char buf[MOVE_BUF_SIZE];  
  format_move(buf, &result->move, engine->xboard_mode);

  PRINT_LOG(&xboard_log, "\nAI > %s", buf);
  
  if(engine->xboard_mode) {
    printf("move %s\n", buf);
  } else {
    print_statistics(engine, result);
    print_prompt(engine);
    printf("%s\n", buf);
    print_game_state(engine);
  }
}

static void print_msg(engine_s *engine, const char *fmt, square_e from, square_e to) {
  if(!engine->xboard_mode) {
    if(from >= 0) {
      char from_buf[POS_BUF_SIZE];
      format_square(from_buf, from);
      if(to >= 0) {
        char to_buf[POS_BUF_SIZE];
        format_square(to_buf, to);
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
int no_piece_at_square(engine_s *engine, square_e square) 
{
  if((square2bit[square] & engine->game.total_a) == 0) {
    print_msg(engine, "There is no piece at %s.\n", square, -1);
    return 1;
  }
  return 0;
}

int move_is_illegal(engine_s *engine, move_s *move)
{
  if(no_piece_at_square(engine, move->from)) {
    return 1;
  }
  if(move->from == move->to) {
    print_msg(engine, "The origin %s is the same as the destination.\n", move->from, -1);
    return 1;
  } 
  if((square2bit[move->from] & get_my_pieces(&engine->game)) == 0) {
    print_msg(engine, "The piece at %s is not your piece.\n", move->from, -1);
    return 1;
  } 
  if((square2bit[move->to] & get_moves(&engine->game, move->from)) == 0) { 
    print_msg(engine, "The piece at %s cannot move to %s.\n", move->from, move->to);
    return 1;
  } 
  return 0;
}


void init_engine(engine_s *engine)
{
  memset(engine, 0, sizeof *engine);
  reset_board(&engine->game);
  engine->xboard_mode = 0;
  engine->resign_delayed = 0;
  engine->game_n = 1;
  engine->waiting = 1;
  engine->mode = ENGINE_PLAYING_AS_BLACK;
  engine->depth = 8;
}

void finished_move(engine_s *engine)
{
  /* It is now the other players turn */
  change_player(&engine->game);
  /* move_n holds number of complete moves, incremented when it is
     white's move */
  if(engine->game.turn == WHITE) {
    engine->move_n++;
  }    
}

static inline void log_ai_move(move_s *move, int captured, int check) {
#ifdef DEBUG
#endif
}


static inline void do_ai_move(engine_s *engine)
{
  start_move_log(engine);

  search_result_s result;
  search(engine->depth, &engine->game, &result);

  int resign = 0;   

  if(engine->resign_delayed) {
    resign = 1;
  } else if(result.move.from == NO_SQUARE) {
    if(engine->game.check) {
      /* Checkmate */
      resign = 1;
    } else {
      /* Stalemate - delay resign to see if draw is given */
      engine->resign_delayed = 1;
      return;
    }
  }
  
  if(resign) {
    mark_time(engine);
    print_ai_resign(engine);
    engine->mode = ENGINE_FORCE_MODE;
    return;
  }
  
  make_move(&engine->game, &result.move);
  mark_time(engine);
  print_ai_move(engine, &result);
  finished_move(engine);
  reset_time(engine);
}

/* Accept a valid user move.
   Return:  0 - Valid move
            1 - Well formed but not valid
	    2 - Not recognised */
static inline int accept_move(engine_s *engine, const char *input)
{
  move_s move;
  
  if(parse_move(input, &move)) {
    return 2;
  }
  if(move_is_illegal(engine, &move)) {
    return 1;
  }
  mark_time(engine);
  /* If waiting after a new game, first user move begins the game as white */
  /* Don't count elapsed time while waiting for first move */
  if(engine->waiting) { 
    engine->waiting = 0;
    reset_time(engine);
  } 	  
  make_move(&engine->game, &move);
  if(is_in_normal_play(engine)) {
    print_statistics(engine, 0);
  }
  print_game_state(engine);
  finished_move(engine);
  return 0;
}

static inline int accept_message(const char *input)
{
  if(input[0] == '{') {
    PRINT_LOG(&xboard_log, "Msg : %s", input+1);
    return 0;
  }
  return 1;
}

static inline void get_user_input(engine_s *engine)
{
  print_prompt(engine);
  const char *input;
  input = get_input();
  if(input[0] == 0) return; 
  if(!accept_message(input)) return;
  if(!accept_command(engine, input)) return;
  if(!accept_move(engine, input)) return;
  print_msg(engine, "Unrecognised command\nEnter 'help' for a list of commands.\n", -1, -1);
}

/*
 *  UI Main Loop
 */
void main_loop(engine_s *engine) 
{
  while(engine->mode != ENGINE_QUIT) {
    if(ai_turn(engine)) {
      do_ai_move(engine);
    } else {
      get_user_input(engine);
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
