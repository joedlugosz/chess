#ifndef IO_H
#define IO_H

#include "board.h"

struct notation_s_;

void print_board(FILE *f, state_s *state, plane_t hl1, plane_t hl2);
void print_plane(FILE *f, plane_t plane, plane_t indicator);
void print_plane_rank(FILE *f, unsigned char rank, unsigned char indicator);
void print_thought_moves(FILE *f, int depth, struct notation_s_ *moves);
const char *get_input(void);
const char *get_delim(char delim);

#endif /* IO_H */