/*
 *  Static position evaluation
 */

#ifndef EVAL_H
#define EVAL_H

/* Position evaluation score */
typedef int score_t;

struct state_s_;
score_t evaluate(struct state_s_ *);

#endif /* EVAL_H */
