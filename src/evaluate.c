/*
 *  Static position evaluation
 *
 *  Uses Shannon's method and scores, x10. Scoring is from the point of view of
 *  the current player, positive when leading.
 */

#include "evaluate.h"

#include <limits.h>
#include <stdlib.h>

#include "debug.h"
#include "options.h"
#include "position.h"

/* Factor to negate the score for black */
const score_t player_factor[N_PLAYERS] = {1, -1};

/*
 *  User options
 */

/* Shannon's weights * 10 */
int piece_weights[N_PIECE_T] = {100, 500, 300, 300, 900, 2000};

/* Other factors in Shannon's method * 10 */
int mobility = 10;
int doubled = 50;
int blocked = 50;

/* Small random value 0-9 */
int randomness = 0;

/* The sum of black and white's material */
int endgame_material = 6000;

/* Evaluation user options. */
const struct option _eval_opts[] = {
    /* clang-format off */
  { "Pawn value",            INT_OPT,  .value.integer = &piece_weights[PAWN],    0, 0, 0 },
  { "Rook value",            INT_OPT,  .value.integer = &piece_weights[ROOK],    0, 0, 0 },
  { "Bishop value",          INT_OPT,  .value.integer = &piece_weights[BISHOP],  0, 0, 0 },
  { "Knight value",          INT_OPT,  .value.integer = &piece_weights[KNIGHT],  0, 0, 0 },
  { "Queen value",           INT_OPT,  .value.integer = &piece_weights[QUEEN],   0, 0, 0 },
  { "King value",            INT_OPT,  .value.integer = &piece_weights[KING],    0, 0, 0 },
  { "Mobility bonus",        INT_OPT,  .value.integer = &mobility,               0, 0, 0 },
  { "Blocked pawn penalty",  INT_OPT,  .value.integer = &blocked,                0, 0, 0 },
  { "Doubled pawn penalty",  INT_OPT,  .value.integer = &doubled,                0, 0, 0 },
  { "Randomness",            SPIN_OPT, .value.integer = &randomness,          0, 2000, 0 },
  { "Endgame material",      INT_OPT,  .value.integer = &endgame_material,       0, 0, 0 },
    /* clang-format on */
};
const struct options eval_opts = {sizeof(_eval_opts) / sizeof(_eval_opts[0]),
                                  _eval_opts};

/*
 *  Functions
 */

/* Evaluate one player's pieces, producing a positive score */
static inline score_t evaluate_player(const struct position *position,
                                      enum player player) {
  int score = 0;
  int pt_first;
  bitboard_t pieces;

  pt_first = N_PIECE_T * player;

  /* Materials */
  for (int i = 0; i < N_PIECE_T; i++) {
    score += piece_weights[i] * pop_count(position->a[i + pt_first]);
  }
  /* Mobility - for each piece count the number of moves */
  pieces = position->player_a[player];
  while (pieces) {
    enum square square = bit2square(take_next_bit_from(&pieces));
    score += mobility * pop_count(get_moves(position, square));
  }
  /* Doubled pawns - look for pawn occupancy of >1 on any rank of the B-stack */
  pieces = position->b[PAWN + pt_first];
  while (pieces) {
    if (pop_count(pieces & 0xffull) > 1) {
      score -= doubled;
    }
    pieces >>= 8;
  }
  /* Blocked pawns - look for pawns with no moves */
  pieces = position->a[PAWN + pt_first];
  while (pieces) {
    enum square square = bit2square(take_next_bit_from(&pieces));
    if (pop_count(get_moves(position, square) == 0ull)) {
      score -= blocked;
    }
  }
  /* Random element */
  if (randomness) {
    score += rand() % randomness;
  }
  return score;
}

int is_endgame(const struct position *position) {
  return (evaluate_player(position, WHITE) + evaluate_player(position, BLACK)) >
         endgame_material;
}

/* Evaluate a position, producing a score which is positive if the current
   player is leading */
score_t evaluate(const struct position *position) {
  return (evaluate_player(position, WHITE) - evaluate_player(position, BLACK)) *
         player_factor[position->turn];
}

/* Tests */
int test_eval(void) {
  struct position position;
  reset_board(&position);
  /* Starting positions should sum to zero */
  ASSERT(evaluate(&position) == 0);
  return 0;
}
