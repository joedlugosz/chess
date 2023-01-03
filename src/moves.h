/*
 *  Functions describing piece moves and relationships
 */
#ifndef MOVES_H
#define MOVES_H

#include "position.h"

bitboard_t get_pawn_moves(struct position *position, enum square square,
                          enum player player);
bitboard_t get_king_moves(struct position *position, enum square from,
                          enum player player);
bitboard_t get_bishop_moves(const struct position *position,
                            enum square a_square);
bitboard_t get_rook_moves(const struct position *position,
                          enum square a_square);
extern bitboard_t knight_moves[64];
bitboard_t get_moves(const struct position *position, enum square from);
void calculate_moves(struct position *position, int player);

#endif
