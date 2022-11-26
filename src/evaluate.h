/*
 *  Static position evaluation
 */

#ifndef EVAL_H
#define EVAL_H

struct position;

/* Position evaluation score */
typedef int score_t;

score_t evaluate(struct position *);

#endif /* EVAL_H */
