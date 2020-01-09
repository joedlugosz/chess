#ifndef HISTORY_H
#define HISTORY_H
#include "search.h"
void write_move_history(search_context_s *, int, pos_t, pos_t, player_e, score_t);
int is_repeated_move(search_context_s *, int, pos_t, pos_t);
#endif /* HISTORY_H */
