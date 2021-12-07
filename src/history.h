/*
 *   Move history
 */

#ifndef HISTORY_H
#define HISTORY_H

#include "evaluate.h"

struct search_job_s_;

void write_move_history(struct search_job_s_ *, int, move_s *, player_e, score_t);
int is_repeated_move(struct search_job_s_ *, int, move_s *);

#endif /* HISTORY_H */
