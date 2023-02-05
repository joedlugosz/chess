/*
 *  Functions describing piece moves and relationships
 */
#ifndef MOVES_H
#define MOVES_H

#include "position.h"

void calculate_moves(struct position *position);
void check_magics();

/* These functions are used by find_magics.c */
bitboard_t unpack_moves(bitboard_t template, bitboard_t packed);
typedef bitboard_t (*generate_blocker_fn)(enum square);
typedef bitboard_t (*generate_moves_fn)(enum square, bitboard_t);
bitboard_t generate_bishop_moves(enum square square, bitboard_t blockers);
bitboard_t generate_rook_moves(enum square square, bitboard_t blockers);
bitboard_t get_rook_moves(bitboard_t occupancy, enum square square);
bitboard_t get_bishop_moves(bitboard_t occupancy, enum square square);
#endif
