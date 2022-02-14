/*
 *   Text IO formatting and parsing
 */

#ifndef IO_H
#define IO_H

#include <sys/types.h>
#include <time.h>

#include "evaluate.h"
#include "search.h"
#include "state.h"

void print_board(state_s *state, bitboard_t hl1, bitboard_t hl2);
void print_plane(bitboard_t plane, bitboard_t indicator);
void print_move(move_s *move);
void print_plane_rank(unsigned char rank, unsigned char indicator);
void print_thought_moves(int depth, move_s *moves);
const char *get_input(void);
void get_input_to_buf(char *buf, size_t buf_size);
const char *get_delim(char delim);
int parse_square(const char *, square_e *);
int parse_move(const char *, move_s *);
int format_square(char *, square_e);
int format_move(char *, move_s *, int);
void xboard_thought(search_job_s *job, int depth, score_t score, clock_t time, int nodes);

#endif /* IO_H */
