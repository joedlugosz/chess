/*
 *  UI and XBoard Interface
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
static inline void reset_time(struct engine *engine) {
  engine->start_time = clock();
  engine->elapsed_time = 0;
}
static inline void mark_time(struct engine *engine) {
  engine->elapsed_time = clock() - engine->start_time;
}
static inline clock_t get_time(struct engine *engine) {
  return engine->elapsed_time;
}

static inline int ai_turn(struct engine *engine) {
  if (engine->game.turn != engine->mode)
    return 0;
  else
    return 1;
}

static inline int is_in_normal_play(struct engine *engine) {
  if (engine->mode >= ENGINE_FORCE_MODE)
    return 0;
  else
    return 1;
}

/*
 *  Output
 */
static inline void print_statistics(struct engine *engine,
                                    struct search_result *result) {
  if (!engine->xboard_mode) {
    double time = (double)(get_time(engine)) / (double)CLOCKS_PER_SEC;
    printf("\n%d : %0.2lf sec", evaluate(&engine->game) / 10, time);
    if (ai_turn(engine) && result) {
      printf(" : %d nodes : b = %0.3lf : %0.2lf knps : %0.2lf%% collisions",
             result->n_leaf, result->branching_factor,
             (double)result->n_leaf / (time * 1000.0), result->collisions);
    }
    printf("\n\n");
  }
}

static inline void print_game_position(struct engine *engine) {
  if (!engine->xboard_mode) {
    print_board(&engine->game, 0, 0);
    if (in_check(&engine->game)) {
      printf("%s is in check.\n", player_text[engine->game.turn]);
    }
  }
}

static inline void print_prompt(struct engine *engine) {
  if (!engine->xboard_mode) {
    if (engine->mode != ENGINE_FORCE_MODE) {
      printf("%s %s> ", player_text[engine->game.turn],
             engine->game.turn == engine->mode ? "(AI) " : "");
    } else {
      printf("FORCE > ");
    }
  }
}

static inline void print_ai_resign(struct engine *engine) {
  if (engine->xboard_mode) {
    printf("resign\n");
  } else {
    printf("\n%s resigns.\n\n", player_text[engine->game.turn]);
  }
}

static inline void print_ai_move(struct engine *engine,
                                 struct search_result *result) {
  ASSERT(is_in_normal_play(engine));

  char buf[MOVE_BUF_SIZE];
  if (engine->xboard_mode) {
    format_move(buf, &result->move, 1);
    printf("move %s\n", buf);
  } else {
    print_statistics(engine, result);
    print_prompt(engine);
    format_move_san(buf, &result->move);
    printf("%s\n", buf);
  }
}

static void print_msg(struct engine *engine, const char *fmt, enum square from,
                      enum square to) {
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
int ui_no_piece_at_square(struct engine *engine, enum square square) {
  if ((square2bit[square] & engine->game.total_a) == 0) {
    print_msg(engine, "There is no piece at %s.\n", square, -1);
    return 1;
  }
  return 0;
}

int move_is_illegal(struct engine *engine, struct move *move) {
  int result = check_legality(&engine->game, move);

  switch (result) {
    case ERR_NO_PIECE:
      print_msg(engine, "There is no piece at %s.\n", move->from, -1);
      break;
    case ERR_SRC_EQUAL_DEST:
      print_msg(engine, "The origin %s is the same as the destination.\n",
                move->from, -1);
      break;
    case ERR_NOT_MY_PIECE:
      print_msg(engine, "The piece at %s is not your piece.\n", move->from, -1);
      break;
    case ERR_CANT_MOVE_THERE:
      print_msg(engine, "The piece at %s cannot move to %s.\n", move->from,
                move->to);
      break;
    default:
      break;
  }

  return result;
}

void init_engine(struct engine *engine) {
  memset(engine, 0, sizeof *engine);
  reset_board(&engine->game);
  history_clear(&engine->history);
  engine->xboard_mode = 0;
  engine->resign_delayed = 0;
  engine->game_n = 1;
  engine->waiting = 1;
  engine->mode = ENGINE_PLAYING_AS_BLACK;
  engine->depth = 8;
}

void finished_move(struct engine *engine) {
  /* It is now the other players turn */
  change_player(&engine->game);
  /* move_n holds number of complete moves, incremented when it is
     white's move */
  if (engine->game.turn == WHITE) {
    engine->move_n++;
  }
}

#ifdef DEBUG
static inline void log_ai_move(struct move *move, int captured, int check) {}
#endif

static inline void do_ai_move(struct engine *engine) {
  struct search_result result;
  search(engine->depth, &engine->history, &engine->game, &result, 1);

  /* If no AI move was found, print checkmate or stalemate messages */
  if (engine->resign_delayed) {
  } else if (result.move.from == result.move.to) {
    if (engine->game.check[engine->game.turn]) {
      mark_time(engine);
      printf("\nCheckmate - %d-%d\n\n", engine->game.check[BLACK],
             engine->game.check[WHITE]);
      print_ai_resign(engine);
      engine->mode = ENGINE_FORCE_MODE;
    } else {
      printf("\nStalemate - 1/2-1/2\n\n");
      engine->mode = ENGINE_FORCE_MODE;
    }
    return;
  }

  /* Make the AI move */
  make_move(&engine->game, &result.move);
  history_push(&engine->history, engine->game.hash, &result.move);
  mark_time(engine);
  print_ai_move(engine, &result);
  finished_move(engine);
  print_game_position(engine);
  reset_time(engine);

  /* Search at depth 1 to see if human has any moves, then print
     checkmate or stalemate messages for human */
  search(1, &engine->history, &engine->game, &result, 0);
  if (result.move.from == result.move.to) {
    if (engine->game.check[engine->game.turn]) {
      printf("\nCheckmate - %d-%d\n\n", engine->game.check[BLACK],
             engine->game.check[WHITE]);
    } else {
      printf("\nStalemate - 1/2-1/2\n\n");
    }
    engine->mode = ENGINE_FORCE_MODE;
  }
}

/* Accept a valid user move.
   Return:  0 - Valid move
            1 - Well formed but not valid
            2 - Not recognised */
static inline int accept_move(struct engine *engine, const char *input) {
  struct move move;

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
  history_push(&engine->history, engine->game.hash, &move);

  if (is_in_normal_play(engine)) {
    print_statistics(engine, 0);
  }
  finished_move(engine);
  print_game_position(engine);
  return 0;
}

static inline int accept_message(const char *input) {
  if (input[0] == '{') {
    return 0;
  }
  return 1;
}

static inline void get_user_input(struct engine *engine) {
  print_prompt(engine);
  const char *input;
  input = get_input();
  if (input[0] == 0) return;
  if (!accept_message(input)) return;
  if (!accept_command(engine, input)) return;
  if (!accept_move(engine, input)) return;
  print_msg(engine,
            "Unrecognised command\nEnter 'help' for a list of commands.\n", -1,
            -1);
}

/*
 *  UI Main Loop
 */
void run_engine(struct engine *engine) {
  if (!engine->xboard_mode) print_program_info();
  print_game_position(engine);

  while (engine->mode != ENGINE_QUIT) {
    if (ai_turn(engine)) {
      do_ai_move(engine);
    } else {
      get_user_input(engine);
    }
  }
}

/* Permanently enter XBoard mode and disable Ctrl-C */
void enter_xboard_mode(struct engine *e) {
  e->xboard_mode = 1;
  ignore_sigint();
}
