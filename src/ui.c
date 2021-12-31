/*
 *  Program entry point and user interface.
 */

#include <ctype.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "commands.h"
#include "debug.h"
#include "engine.h"
#include "info.h"
#include "io.h"
#include "os.h"
#include "search.h"

enum { POS_BUF_SIZE = 3, MOVE_BUF_SIZE = 10 };

/*
 *  Clocks
 */
static inline void reset_time(engine_s *engine) {
  engine->start_time = clock();
  engine->elapsed_time = 0;
}
static inline void mark_time(engine_s *engine) {
  engine->elapsed_time = clock() - engine->start_time;
}
static inline clock_t get_time(engine_s *engine) { return engine->elapsed_time; }

static inline int ai_turn(engine_s *engine) {
  if (engine->game.turn != engine->mode)
    return 0;
  else
    return 1;
}

static inline int is_in_normal_play(engine_s *engine) {
  if (engine->mode >= ENGINE_FORCE_MODE)
    return 0;
  else
    return 1;
}

/*
 *  Output
 */
static inline void print_statistics(engine_s *engine, search_result_s *result) {
  if (!engine->xboard_mode) {
    double time = (double)(get_time(engine)) / (double)CLOCKS_PER_SEC;
    printf("\n%d : %0.2lf sec", evaluate(&engine->game) / 10, time);
    if (ai_turn(engine) && result) {
      printf(" : %d nodes : b = %0.3lf : %0.2lf knps", result->n_leaf, result->branching_factor,
             (double)result->n_leaf / (time * 1000.0));
    }
    printf("\n\n");
  }
}

static inline void print_game_state(engine_s *engine) {
  if (!engine->xboard_mode) {
    print_board(&engine->game, 0, 0);
    if (in_check(&engine->game)) {
      printf("%s is in check.\n", player_text[engine->game.turn]);
    }
  }
}

static inline void print_prompt(engine_s *engine) {
  if (!engine->xboard_mode) {
    if (engine->mode != ENGINE_FORCE_MODE) {
      printf("%s %s> ", player_text[engine->game.turn],
             engine->game.turn == engine->mode ? "(AI) " : "");
    } else {
      printf("FORCE > ");
    }
  }
}

static inline void print_ai_resign(engine_s *engine) {
  if (engine->xboard_mode) {
    printf("resign\n");
  } else {
    printf("%s resigns.", player_text[engine->game.turn]);
  }
}

static inline void print_ai_move(engine_s *engine, search_result_s *result) {
  ASSERT(is_in_normal_play(engine));

  char buf[MOVE_BUF_SIZE];
  format_move(buf, &result->move, engine->xboard_mode);

  if (engine->xboard_mode) {
    printf("move %s\n", buf);
  } else {
    print_statistics(engine, result);
    print_prompt(engine);
    printf("%s\n", buf);
  }
}

static void print_msg(engine_s *engine, const char *fmt, square_e from, square_e to) {
  if (!engine->xboard_mode) {
    if (from >= 0) {
      char from_buf[POS_BUF_SIZE];
      format_square(from_buf, from);
      if (to >= 0) {
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
int ui_no_piece_at_square(engine_s *engine, square_e square) {
  if ((square2bit[square] & engine->game.total_a) == 0) {
    print_msg(engine, "There is no piece at %s.\n", square, -1);
    return 1;
  }
  return 0;
}

int move_is_illegal(engine_s *engine, move_s *move) {
  int result = check_legality(&engine->game, move);

  switch (result) {
    case ERR_NO_PIECE:
      print_msg(engine, "There is no piece at %s.\n", move->from, -1);
      break;
    case ERR_SRC_EQUAL_DEST:
      print_msg(engine, "The origin %s is the same as the destination.\n", move->from, -1);
      break;
    case ERR_NOT_MY_PIECE:
      print_msg(engine, "The piece at %s is not your piece.\n", move->from, -1);
      break;
    case ERR_CANT_MOVE_THERE:
      print_msg(engine, "The piece at %s cannot move to %s.\n", move->from, move->to);
      break;
    default:
      break;
  }
  
  return result;
}

void init_engine(engine_s *engine) {
  memset(engine, 0, sizeof *engine);
  reset_board(&engine->game);
  engine->xboard_mode = 0;
  engine->resign_delayed = 0;
  engine->game_n = 1;
  engine->waiting = 1;
  engine->mode = ENGINE_PLAYING_AS_BLACK;
  engine->depth = 8;
}

void finished_move(engine_s *engine) {
  /* It is now the other players turn */
  change_player(&engine->game);
  /* move_n holds number of complete moves, incremented when it is
     white's move */
  if (engine->game.turn == WHITE) {
    engine->move_n++;
  }
}

#ifdef DEBUG
static inline void log_ai_move(move_s *move, int captured, int check) {}
#endif

static inline void do_ai_move(engine_s *engine) {
  //  start_move_log(engine);

  search_result_s result;
  search(engine->depth, &engine->game, &result);

  int resign = 0;

  if (engine->resign_delayed) {
    resign = 1;
  } else if (result.move.from == NO_SQUARE) {
    if (engine->game.check[engine->game.turn]) {
      /* Checkmate */
      resign = 1;
    } else {
      /* Stalemate - delay resign to see if draw is given */
      engine->resign_delayed = 1;
      return;
    }
  }

  if (resign) {
    mark_time(engine);
    print_ai_resign(engine);
    engine->mode = ENGINE_FORCE_MODE;
    return;
  }

  make_move(&engine->game, &result.move);
  mark_time(engine);
  print_ai_move(engine, &result);
  finished_move(engine);
  print_game_state(engine);
  reset_time(engine);
}

/* Accept a valid user move.
   Return:  0 - Valid move
            1 - Well formed but not valid
            2 - Not recognised */
static inline int accept_move(engine_s *engine, const char *input) {
  move_s move;

  if (parse_move(input, &move)) {
    return 2;
  }
  if (move_is_illegal(engine, &move)) {
    return 1;
  }
  mark_time(engine);
  /* If waiting after a new game, first user move begins the game as white */
  /* Don't count elapsed time while waiting for first move */
  if (engine->waiting) {
    engine->waiting = 0;
    reset_time(engine);
  }
  make_move(&engine->game, &move);
  if (is_in_normal_play(engine)) {
    print_statistics(engine, 0);
  }
  finished_move(engine);
  print_game_state(engine);
  return 0;
}

static inline int accept_message(const char *input) {
  if (input[0] == '{') {
    return 0;
  }
  return 1;
}

static inline void get_user_input(engine_s *engine) {
  print_prompt(engine);
  const char *input;
  input = get_input();
  if (input[0] == 0) return;
  if (!accept_message(input)) return;
  if (!accept_command(engine, input)) return;
  if (!accept_move(engine, input)) return;
  print_msg(engine, "Unrecognised command\nEnter 'help' for a list of commands.\n", -1, -1);
}

/*
 *  UI Main Loop
 */
void run_engine(engine_s *engine) {
  if (!engine->xboard_mode) print_program_info();
  print_game_state(engine);

  while (engine->mode != ENGINE_QUIT) {
    if (ai_turn(engine)) {
      do_ai_move(engine);
    } else {
      get_user_input(engine);
    }
  }
}

/* Permanently enter XBoard mode and disable Ctrl-C */
void enter_xboard_mode(engine_s *e) {
  e->xboard_mode = 1;
  ignore_sigint();
}
