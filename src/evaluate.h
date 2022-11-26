/*
 *  Static position evaluation
 */

#ifndef EVAL_H
#define EVAL_H

/* Position evaluation score */
typedef int score_t;

struct struct position_;
score_t evaluate(struct struct position_ *);

#endif /* EVAL_H */
