/*
 *   Text IO formatting and parsing
 */

#ifndef IO_H
#define IO_H

#include "state.h"

void print_board(FILE *f, state_s *state, bitboard_t hl1, bitboard_t hl2);
void print_plane(FILE *f, bitboard_t plane, bitboard_t indicator);
void print_plane_rank(FILE *f, unsigned char rank, unsigned char indicator);
void print_thought_moves(FILE *f, int depth, move_s *moves);
const char *get_input(void);
const char *get_delim(char delim);
int parse_square(const char *, square_e *);
int parse_move(const char *, move_s *);
int format_square(char *, square_e);
int format_move(char *, move_s *, int);

#endif /* IO_H */
