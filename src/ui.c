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

/* Buffer sizes for move and position*/
enum { POS_BUF_SIZE = 3, MOVE_BUF_SIZE = 10 };

/*
 *  Game clocks
 */

/* Reset the elapsed time and set the starting of the next interval. */
static inline void reset_time(struct engine *engine) {
  engine->start_time = clock();
  engine->elapsed_time = 0;
}

/* Mark the elapsed time since the last call to `reset_time`. */
static inline void mark_time(struct engine *engine) {
  engine->elapsed_time = clock() - engine->start_time;
}

/*
 *  Truth checking
 */

/* Return true if it is the AI's turn to move.*/
static inline int is_ai_turn(struct engine *engine) {
  return (engine->game.turn == engine->mode);
}

/* Return true if in normal game play, e.g. not force mode */
static inline int is_in_normal_play(struct engine *engine) {
  return (engine->mode < ENGINE_FORCE_MODE);
}

/*
 *  Printing output
 */

/* Print statistics about a completed turn.  For human moves, print the score
 * and time.  For AI moves, also print search statistics. */
static inline void print_statistics(struct engine *engine,
                                    struct search_result *result) {
  if (!engine->xboard_mode) {
    double time = (double)(engine->elapsed_time) / (double)CLOCKS_PER_SEC;
    printf("\n%d : %0.2lf sec", evaluate(&engine->game) / 10, time);
    if (is_ai_turn(engine) && result) {
      printf(" : %d nodes : b = %0.3lf : %0.2lf knps : %0.2lf%% collisions",
             result->n_leaf, result->branching_factor,
             (double)result->n_leaf / (time * 1000.0), result->collisions);
    }
    printf("\n\n");
  }
}

/* Print the state of the game including board and check. */
static inline void print_game_state(struct engine *engine) {
  if (!engine->xboard_mode) {
    print_board(&engine->game, 0, 0);
    if (in_check(&engine->game)) {
      printf("%s is in check.\n", player_text[engine->game.turn]);
    }
  }
}

/* Print a prompt for user input showing the moving side or force mode,
   or suffixed with "(AI)" to indicate AI output. */
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

/* Print a resignation message. */
static inline void print_ai_resign(struct engine *engine) {
  if (engine->xboard_mode) {
    printf("resign\n");
  } else {
    printf("\n%s resigns.\n\n", player_text[engine->game.turn]);
  }
}

static inline void print_ai_stalemate(struct engine *engine) {
  // PRINT_LOG(&xboard_log, "%s", "\nAI > stalemate");
  if (engine->xboard_mode) {
    printf("RESULT 1/2-1/2 {Stalemate}\n");
  } else {
    printf("Stalemate - %s offers a draw.", player_text[engine->game.turn]);
  }
}

/* Print the AI's move. */
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

/* Print a checkmate score message */
static inline void print_checkmate_message(const struct engine *engine) {
  printf("\nCheckmate - %d-%d\n\n", engine->game.check[BLACK],
         engine->game.check[WHITE]);
}

/* Print a stalemate score message */
static inline void print_stalemate_message() {
  printf("\nStalemate - 1/2-1/2\n\n");
}

/* Print a formatted error message about a move, of the form
 * ("....%s....%s...", from, to) */
static void print_move_error_msg(struct engine *engine, const char *fmt,
                                 enum square from, enum square to) {
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

/* Check that a piece exists at `square`.  Print an error message if necessary.
 * Return 0 if OK.  Used to validate user input in `commands.c` */
int ui_no_piece_at_square(struct engine *engine, enum square square) {
  if ((square2bit[square] & engine->game.total_a) == 0) {
    print_move_error_msg(engine, "There is no piece at %s.\n", square, -1);
    return 1;
  }
  return 0;
}

/* Check move legality and print error message if necessary.  Return 0 if OK. */
int ui_check_legality(struct engine *engine, struct move *move) {
  int result = check_legality(&engine->game, move);
  switch (result) {
    case ERR_NO_PIECE:
      print_move_error_msg(engine, "There is no piece at %s.\n", move->from,
                           -1);
      break;
    case ERR_SRC_EQUAL_DEST:
      print_move_error_msg(engine,
                           "The origin %s is the same as the destination.\n",
                           move->from, -1);
      break;
    case ERR_NOT_MY_PIECE:
      print_move_error_msg(engine, "The piece at %s is not your piece.\n",
                           move->from, -1);
      break;
    case ERR_CANT_MOVE_THERE:
      print_move_error_msg(engine, "The piece at %s cannot move to %s.\n",
                           move->from, move->to);
      break;
    default:
      break;
  }

  return result;
}

/*
 *  AI actions
 */

/* Make the AI move and set the UI up for the next user move. */
static inline void do_ai_turn(struct engine *engine) {
  /* Search for AI move */
  struct search_result result;
  search(engine->depth, &engine->history, &engine->game, &result, 1);

  /* If no AI move was found, print checkmate or stalemate messages and end the
   * game. */
  if (result.move.from == result.move.to) {
    if (engine->game.check[engine->game.turn]) {
      mark_time(engine);
      print_checkmate_message(engine);
      print_ai_resign(engine);
      engine->mode = ENGINE_FORCE_MODE;
    } else {
      /* Stalemate - delay resign to see if draw is given */
      engine->resign_delayed = 1;
      print_ai_stalemate(engine);
    }
    return;
  }

  /* Make the AI move */
  make_move(&engine->game, &result.move);
  history_push(&engine->history, engine->game.hash, &result.move);
  mark_time(engine);
  print_ai_move(engine, &result);
  change_player(&engine->game);
  print_game_state(engine);
  reset_time(engine);

  /* Search at depth 1 to see if human has any moves.  If not, print
     checkmate or stalemate messages for human and end the game. */
  search(1, &engine->history, &engine->game, &result, 0);
  if (result.move.from == result.move.to) {
    if (engine->game.check[engine->game.turn]) {
      print_checkmate_message(engine);
    } else {
      print_stalemate_message();
    }
    engine->mode = ENGINE_FORCE_MODE;
  }
}

/*
 *  User actions
 */

/* Accept a valid user move.  Make the move and print statistics and the new
 * position.  Return 0 if accepted. */
static inline int accept_move(struct engine *engine, const char *input) {
  struct move move;

  if (parse_move(input, &move)) return 1;
  if (ui_check_legality(engine, &move)) return 1;

  mark_time(engine);
  /* If waiting after a new game when the user is white, the first user move
   * begins the game.  Reset the elapsed time so that it is not counted while
   * waiting for the move. */
  if (engine->waiting) {
    engine->waiting = 0;
    reset_time(engine);
  }
  make_move(&engine->game, &move);
  history_push(&engine->history, engine->game.hash, &move);

  if (is_in_normal_play(engine)) {
    print_statistics(engine, 0);
  }
  change_player(&engine->game);
  print_game_state(engine);
  return 0;
}

/* Accept a message phrase beginning with `{`. Return 0 if accepted.  */
static inline int accept_message(const char *input) {
  if (input[0] == '{') {
    return 0;
  }
  return 1;
}

/* Get and process the next phrase of user input */
static inline void process_user_input(struct engine *engine) {
  print_prompt(engine);
  const char *input;
  input = get_input();
  if (input[0] == 0) return;
  if (!accept_message(input)) return;
  if (!accept_command(engine, input)) return;
  if (!accept_move(engine, input)) return;
  print_move_error_msg(
      engine, "Unrecognised command\nEnter 'help' for a list of commands.\n",
      -1, -1);
}

/*
 *  Setup and run
 */

/* UI Main Loop */
void run_engine(struct engine *engine) {
  if (!engine->xboard_mode) print_program_info();
  print_game_state(engine);

  while (engine->mode != ENGINE_QUIT) {
    if (is_ai_turn(engine))
      do_ai_turn(engine);
    else
      process_user_input(engine);
  }
}

/* Permanently enter XBoard mode and disable Ctrl-C */
void enter_xboard_mode(struct engine *engine) {
  engine->xboard_mode = 1;
  ignore_sigint();
}

/* Initialise the module.  Called at startup. */
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
