/*
 *  Searching
 */

#ifndef SEARCH_H
#define SEARCH_H

#include <time.h>

#include "debug.h"
#include "evaluate.h"
#include "position.h"

/* linked list */
struct move_list {
  struct move move;
  score_t score;
  struct move_list *next;
};

struct search_result {
  score_t score;
  int n_leaf;
  double branching_factor;
  double collisions;
  struct move move;
  clock_t time;
};

enum { SEARCH_DEPTH_MAX = 30, REPEAT_HISTORY_SIZE = 300, N_MOVES = 218 };

struct history;
struct search_job {
  /* Parameters */
  int depth; /* Search depth before quiescence */
  int halt;  /* Halt search */
  int show_thoughts;
  /* position */
  clock_t start_time;
  struct move search_history[SEARCH_DEPTH_MAX];
  struct history *history;
  struct move killer_moves[SEARCH_DEPTH_MAX];
  int n_ai_moves;
  /* Results */
  struct search_result result;
};

void search(int depth, struct history *history, struct position *position,
            struct search_result *result, int show_thoughts);

#endif  // SEARCH_H
