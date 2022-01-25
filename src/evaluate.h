/*
 *  Static position evaluation
 */

#ifndef EVALUATE_H
#define EVALUATE_H

#include "position.h"

/* Position evaluation score */
typedef int score_t;

void evaluate_init();
score_t evaluate(const struct position *position);
int is_endgame(const struct position *position);
score_t evaluate_player_pawns(struct position *position, enum player player);

extern int piece_weights[N_PIECE_T];
extern int mobility;

static inline int opening_pieces_left(const struct position *position,
                                      enum player player) {
  bitboard_t back_row = 0x66ull << ((player == WHITE) ? 0 : 56);
  return pop_count(position->player_a[player] & back_row);
}

static inline int has_queen_moved(const struct position *position,
                                  enum player player) {
  bitboard_t queen_bit = 0x08ull << ((player == WHITE) ? 0 : 56);
  return ((position->player_a[player] & queen_bit) == 0);
}

#endif /* EVALUATE_H */
