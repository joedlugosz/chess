#ifndef EVAL_H
#define EVAL_H

#include "board.h"

/* Evaluation score */
typedef int score_t;
struct state_s_;
int eval(struct state_s_ *state);

#endif /* EVAL_H */
