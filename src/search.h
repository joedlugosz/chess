/*
 *  Searching
 */

#ifndef SEARCH_H
#define SEARCH_H

#include "evaluate.h"
#include "log.h"

typedef struct notation_s_
{
  pos_t from;
  pos_t to;
  piece_e promotion;
  score_t score;
  player_e player;
  status_t captured : 1;
  status_t check : 1;
} notation_s;

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
  double cutoff;
  move_s best_move;
} search_result_s;

enum {
  SEARCH_DEPTH_MAX = 30,
  REPEAT_HISTORY_SIZE = 300,
  N_MOVES = 218
};

typedef struct search_context_s_ {
  /* Parameters */
  int horizon_depth; /* Search depth before quiescence */
  int halt;          /* Halt search */
  /* State */
  notation_s search_history[SEARCH_DEPTH_MAX];
  notation_s repeat_history[REPEAT_HISTORY_SIZE];
  /* Results */
  move_s *best_move;
  score_t score;    /* Score of chosen move */
  int n_searched;   /* Number of nodes searched */
  int n_possible;
  int n_beta;       /* Number of beta cutoffs */ 
  int n_ai_moves;
} search_context_s;

void do_search(int, state_s *, search_result_s *);

#endif // SEARCH_H
