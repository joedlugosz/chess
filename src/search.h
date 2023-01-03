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
  int n_node;
  int n_check_moves;
  int seldep;
  double branching_factor;
  double collisions;
  struct move move;
  enum {
    SEARCH_RESULT_PLAY,
    SEARCH_RESULT_CHECKMATE,
    SEARCH_RESULT_STALEMATE,
    SEARCH_RESULT_DRAW_BY_REPETITION,
  } type;
  double time;
};

enum { SEARCH_DEPTH_MAX = 30, REPEAT_HISTORY_SIZE = 300, N_MOVES = 218 };

struct history;
struct search_job {
  /* Parameters */
  int depth; /* Search depth before quiescence */
  int halt;  /* Halt search */
  int show_thoughts;
  /* position */
  double start_time;
  struct move search_history[SEARCH_DEPTH_MAX];
  struct history *history;
  struct move killer_moves[SEARCH_DEPTH_MAX];
  int n_ai_moves;
  /* Results */
  struct search_result result;
};

void search(int target_depth, double time_budget, double time_margin,
            struct history *history, struct position *position,
            struct search_result *result, int show_thoughts);

#endif  // SEARCH_H
