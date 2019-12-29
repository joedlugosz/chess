#ifndef HISTORY_H
#define HISTORY_H
#include "search.h"
void write_history(search_s *ctx, int depth, pos_t from, pos_t to, player_e to_move, score_t score);
int check_repeat(search_s *ctx, int depth, pos_t from, pos_t to);
#endif /* HISTORY_H */
