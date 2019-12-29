/*
 *  movegen.h
 *
 *  Move generator and perft
 *
 *  5.0    02/2019   First version
 */

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "search.h"

/* Perft statistics */
typedef struct perft_s_ {
  unsigned long long moves;
  unsigned long captures;
  unsigned long en_passant;
  unsigned long castles;
  unsigned long checks;
  unsigned long checkmates;
} perft_s;

int gen_moves(state_s *state, move_s **move_buf_head);
void perft(perft_s *data, state_s *state, int depth);

#endif /* MOVEGEN_H */
