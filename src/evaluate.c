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

/* Front spans - used for passed pawn evaluation.  The set of squares in front
 * of a pawn that must not be blocked by, or under attack from, an opponent's
 * pawn, in order for the pawn to be counted as passed.  This is all the squares
 * forward from the pawn and in the file either side.  Calculated by
 * `evaluate_init`. */
bitboard_t front_spans[N_PLAYERS][N_SQUARES];

/* Factor to negate the score for black */
const score_t player_factor[N_PLAYERS] = {1, -1};

#define OPT_EVAL_PASSED 1
#define OPT_OPENING_GUIDE 1

/*
 *  User options
 */

/* Shannon's weights * 10 */
int piece_weights[N_PIECE_T] = {100, 500, 300, 300, 900, 2000};

/* Other factors in Shannon's method * 10 */
int mobility_bonus = 10;
int doubled_pawn_penalty = 50;
int blocked_pawn_penalty = 50;
int passed_pawn_advance_bonus = 50;

/* Small random value 0-9 */
int randomness = 0;

/* The threshold for the sum of black and white's material to indicate the
 * endgame phase */
int endgame_material = 6000;

/* Penalties for opening up with Queen early */
int unmoved_penalty = 400;
int queen_penalty = 800;

/* Evaluation user options. */
const struct option _eval_opts[] = {
    /* clang-format off */
  { "Pawn value",            INT_OPT,  .value.integer = &piece_weights[PAWN],    0, 0, 0 },
  { "Rook value",            INT_OPT,  .value.integer = &piece_weights[ROOK],    0, 0, 0 },
  { "Bishop value",          INT_OPT,  .value.integer = &piece_weights[BISHOP],  0, 0, 0 },
  { "Knight value",          INT_OPT,  .value.integer = &piece_weights[KNIGHT],  0, 0, 0 },
  { "Queen value",           INT_OPT,  .value.integer = &piece_weights[QUEEN],   0, 0, 0 },
  { "King value",            INT_OPT,  .value.integer = &piece_weights[KING],    0, 0, 0 },
  { "Mobility bonus",        INT_OPT,  .value.integer = &mobility_bonus,         0, 0, 0 },
  { "Blocked pawn penalty",  INT_OPT,  .value.integer = &blocked_pawn_penalty,   0, 0, 0 },
  { "Doubled pawn penalty",  INT_OPT,  .value.integer = &doubled_pawn_penalty,   0, 0, 0 },
  { "Randomness",            SPIN_OPT, .value.integer = &randomness,          0, 2000, 0 },
#if (OPT_OPENING_GUIDE == 1)
  { "Opening unmoved piece penalty", INT_OPT, .value.integer = &unmoved_penalty, 0, 0, 0 },
  { "Opening queen move penalty",    INT_OPT, .value.integer = &queen_penalty,   0, 0, 0 },
#endif
  { "Endgame material threshold",    INT_OPT, .value.integer = &endgame_material, 0, 0, 0 },
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

  enum piece player_first_piece = N_PIECE_T * player;
  enum piece opponent_first_piece = N_PIECE_T * !player;

  /* Materials - score the number of each piece type according to
   * `piece_weights` */
  for (int i = 0; i < N_PIECE_T; i++) {
    score += piece_weights[i] * pop_count(position->a[i + player_first_piece]);
  }

  /* Mobility - a bonus for each possible move. */
  bitboard_t my_pieces = position->player_a[player];
  bitboard_t pieces = my_pieces;
  while (pieces) {
    enum square square = bit2square(take_next_bit_from(&pieces));
    score +=
        mobility_bonus * pop_count(get_moves(position, square) & ~my_pieces);
  }

  /* Doubled pawns - look for pawn occupancy of >1 on any rank of the A-stack */
  pieces = position->a[PAWN + player_first_piece];
  while (pieces) {
    if (pop_count(pieces & 0x0101010101010101ull) > 1)
      score -= doubled_pawn_penalty;
    pieces &= ~0x0101010101010101ull;
    pieces >>= 1;
  }

  /* Blocked and passed pawns */
  pieces = position->a[PAWN + player_first_piece];
  while (pieces) {
    enum square square = bit2square(take_next_bit_from(&pieces));

    /* Penalise blocked pawns which have no moves. */
    if (pop_count(get_moves(position, square) & ~my_pieces) == 0ull)
      score -= blocked_pawn_penalty;

    /* If the pawn is a passed pawn, reward its advancement across the board to
     * encourage promotion even when promotion is beyond the search horizon. */
    if (OPT_EVAL_PASSED && !(front_spans[player][square] &
                             position->a[PAWN + opponent_first_piece])) {
      int file = square / 8;
      int advancement = player ? (6 - file) : (file - 1);
      score += advancement * passed_pawn_advance_bonus;
    }
  }

  /* Random element */
  if (randomness) {
    score += rand() % randomness;
  }

  /* Penalise moving queen before other pieces */
  if (OPT_OPENING_GUIDE && position->phase == OPENING) {
    score -= opening_pieces_left(position, player) * unmoved_penalty;
    if (has_queen_moved(position, player)) score -= queen_penalty;
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

/* Initialise the module. */
void evaluate_init() {
  /*
   * Calculate `front_spans`.  A mask of all squares in the pawn's file, and all
   * squares in the files on either side is shifted so that it co-incides with
   * the pawn's front span.  For extreme files, a mask is created including only
   * the one relevant file.
   */
  for (int square = A1; square <= H7; square++) {
    int rank = square & 7;
    int file = square & ~7;
    bitboard_t fs;
    if (rank == 0)
      fs = 0x0303030303030303ull;
    else if (rank == 7)
      fs = 0xc0c0c0c0c0c0c0c0ull;
    else
      fs = 0x0707070707070707ull << (rank - 1);
    front_spans[WHITE][square] = fs << (file + 8);
    front_spans[BLACK][square] = fs >> (64 - file);
  }
}

/* Tests */
int test_eval(void) {
  struct position position;
  reset_board(&position);
  /* Starting positions should sum to zero */
  ASSERT(evaluate(&position) == 0);
  return 0;
}
