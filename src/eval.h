#ifndef EVAL_H
#define EVAL_H

#include "state.h"

/* Evaluation score */
typedef int score_t;
struct state_s_;
score_t evaluate(struct state_s_ *);

#endif /* EVAL_H */
