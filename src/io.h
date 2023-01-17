/*
 *   Text IO formatting and parsing
 */

#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#include "evaluate.h"
#include "position.h"
#include "search.h"

struct pv;

void print_board(const struct position *position, bitboard_t hl1,
                 bitboard_t hl2);
void print_plane(bitboard_t plane, bitboard_t indicator);
void print_move(const struct move *move);
void print_plane_rank(unsigned char rank, unsigned char indicator);
void print_pv(FILE *out, const struct pv *pv);
const char *get_input(void);
void get_input_to_buf(char *buf, size_t buf_size);
const char *get_delim(char delim);
int parse_square(const char *in, enum square *square);
int parse_move(const char *in, struct move *move);
int format_square(char *out, enum square);
int format_move(char *out, struct move *move, int bare);
int format_move_san(char *out, struct move *move);
void xboard_thought(struct search_job *job, struct pv *pv, int depth,
                    score_t score, double time, int nodes, double knps,
                    int seldep);

#endif /* IO_H */
