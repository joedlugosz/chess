/*
 *  Static evaluation
 */

#include "state.h"
#include "log.h"
#include "evaluate.h"

#include <stdlib.h>
#include <limits.h>

const score_t player_factor[N_PLAYERS] = { 1, -1 };

/* 
 *  Options 
 */

/* Shannon's weights * 10 */
int piece_weights[N_PIECE_T] = { 100, 500, 300, 300, 900, 2000 };

/* Other factors in Shannon's method * 10 */
int mobility = 10;
int doubled = 50;
int blocked = 50;

/* Small random value 0-9 */
int randomness = 0;

/* All the options can be changed */
enum { 
  N_EVAL_OPTS = N_PIECE_T + 4
};
const option_s _eval_opts[N_EVAL_OPTS] = { 
  { "Pawn value",            INT_OPT,  piece_weights + PAWN,    0, 0, 0 },
  { "Rook value",            INT_OPT,  piece_weights + ROOK,    0, 0, 0 },
  { "Bishop value",          INT_OPT,  piece_weights + BISHOP,  0, 0, 0 },
  { "Knight value",          INT_OPT,  piece_weights + KNIGHT,  0, 0, 0 },
  { "Queen value",           INT_OPT,  piece_weights + QUEEN,   0, 0, 0 },
  { "King value",            INT_OPT,  piece_weights + KING,    0, 0, 0 },
  { "Mobility bonus",        INT_OPT,  &mobility,   0, 0    , 0 },
  { "Blocked pawn penalty",  INT_OPT,  &blocked,    0, 0    , 0 },
  { "Doubled pawn penalty",  INT_OPT,  &doubled,    0, 0    , 0 },
  { "Randomness",            SPIN_OPT, &randomness, 0, 2000 , 0 },
};
const options_s eval_opts = { N_EVAL_OPTS, _eval_opts };

/*
 *  Functions
 */

/* Evaluate one player's pieces */
static inline score_t evaluate_player(state_s *state, player_e player)
{
  int score = 0;
  int pt_first;
  bitboard_t pieces;

  pt_first = N_PIECE_T * player;

  /* Materials */
  for(int i = 0; i < N_PIECE_T; i++) {
    score += piece_weights[i] * pop_count(state->a[i + pt_first]);
  }
  /* Mobility - for each piece count the number of moves */
  pieces = state->player_a[player];
  while(pieces) {
    square_e pos = bit2square(take_next_bit_from(&pieces));
    score += mobility * pop_count(get_moves(state, pos));
  }
  /* Doubled pawns - look for pawn occupancy of >1 on any rank of the B-stack */
  pieces = state->b[PAWN + pt_first];
  while(pieces) {
    if(pop_count(pieces & 0xffull) > 1) {
      score -= doubled;
    }
    pieces >>= 8;
  } 
  /* Blocked pawns - look for pawns with no moves */
  pieces = state->a[PAWN + pt_first];
  while(pieces) {
    square_e pos = bit2square(take_next_bit_from(&pieces));
    if(pop_count(get_moves(state, pos) == 0ull)) {
      score -= blocked;
    }
  }
  /* Random element */
  if(randomness) {
    score += rand() % randomness;
  }  
  return score;
}

/* Evalate the position */
score_t evaluate(state_s *state)
{
  return (evaluate_player(state, WHITE) - evaluate_player(state, BLACK)) 
    * player_factor[state->turn];
}

/* Tests */
int test_eval(void)
{
  state_s state;
  reset_board(&state);
  /* Starting positions should sum to zero */
  ASSERT(evaluate(&state) == 0);
  return 0;
}
