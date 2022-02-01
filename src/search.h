/*
 *  Searching
 */

#ifndef SEARCH_H
#define SEARCH_H

#include <time.h>

#include "debug.h"
#include "evaluate.h"

/* linked list */
typedef struct movelist_s_ {
  move_s move;
  score_t score;
  struct movelist_s_ *next;
} movelist_s;

typedef struct search_result_s_ {
  score_t score;
  int n_searched;
  int n_possible;
  int n_beta; /* Number of beta cutoffs */
  double cutoff;
  move_s move;
  clock_t time;
} search_result_s;

enum { SEARCH_DEPTH_MAX = 30, REPEAT_HISTORY_SIZE = 300, N_MOVES = 218 };

typedef struct search_job_s_ {
  /* Parameters */
  int depth; /* Search depth before quiescence */
  int halt;  /* Halt search */
  /* State */
  clock_t start_time;
  move_s search_history[SEARCH_DEPTH_MAX];
  int n_ai_moves;
  /* Results */
  search_result_s result;
} search_job_s;

void search(int, state_s *, search_result_s *);

#endif  // SEARCH_H
