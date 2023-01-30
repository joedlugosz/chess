/*
 *  Functions describing piece moves and relationships
 */
#ifndef MOVES_H
#define MOVES_H

struct position;

void calculate_moves(struct position *position);
void check_magics();

#endif
