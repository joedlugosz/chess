#ifndef HISTORY_H
#define HISTORY_H
#include "search.h"
void write_move_history(search_context_s *, int, move_s *, player_e, score_t);
int is_repeated_move(search_context_s *, int, move_s *);
#endif /* HISTORY_H */
