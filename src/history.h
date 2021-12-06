#ifndef HISTORY_H
#define HISTORY_H

#include "eval.h"

struct search_context_s_;

void write_move_history(struct search_context_s_ *, int, move_s *, player_e, score_t);
int is_repeated_move(struct search_context_s_ *, int, move_s *);

#endif /* HISTORY_H */
