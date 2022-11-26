/*
 *   Text IO formatting and parsing
 */

#ifndef IO_H
#define IO_H

#include <stdio.h>
#include <sys/types.h>
#include <time.h>

#include "evaluate.h"
#include "search.h"
#include "position.h"

struct pv;

void print_board(struct position *position, bitboard_t hl1, bitboard_t hl2);
void print_plane(bitboard_t plane, bitboard_t indicator);
void print_move(struct move *move);
void print_plane_rank(unsigned char rank, unsigned char indicator);
void print_pv(FILE *f, struct pv *pv);
const char *get_input(void);
void get_input_to_buf(char *buf, size_t buf_size);
const char *get_delim(char delim);
int parse_square(const char *, enum square *);
int parse_move(const char *, struct move *);
int format_square(char *, enum square);
int format_move(char *, struct move *, int);
int format_struct movean(char *buf, struct move *move);
void xboard_thought(search_job_s *job, struct pv *pv, int depth, score_t score, clock_t time,
                    int nodes);

#endif /* IO_H */
