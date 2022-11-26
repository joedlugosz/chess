/*
 *   Move list generation, sorting, and perft
 */

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "search.h"

/* Perft statistics */
typedef struct perft_s_ {
  unsigned long long moves;
  unsigned long captures;
  unsigned long promotions;
  unsigned long en_passant;
  unsigned long ep_captures;
  unsigned long castles;
  unsigned long checks;
  unsigned long checkmates;
} perft_s;

int generate_search_movelist(struct position *s, struct move_list **);
int generate_quiescence_movelist(struct position *s, struct move_list **);
void perft_total(struct position *position, int depth);
void perft_divide(struct position *position, int depth);

#endif /* MOVEGEN_H */
