/*
 *  Common Search Functions
 *
 *  4.3    - Created, taken from individual search files
 *  5.0    - Split header files
 */

#ifndef SEARCH_H
#define SEARCH_H

#include "eval.h"
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

/*
typedef enum search_status_e_ {
  NO_MOVE = 0,
  MOVED,
  STALEMATE,
  CHECKMATE
} search_status_e;
*/

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
  int depth;        /* Search depth before quiescence */
  int halt;         /* Halt search */
  /* State */
  notation_s search_history[SEARCH_DEPTH_MAX];
  notation_s repeat_history[REPEAT_HISTORY_SIZE];
  /* Results */
  notation_s best_move;
  //search_status_e status;    /* Search status */
  score_t score;    /* Score of chosen move */
  int n_searched;   /* Number of nodes searched */
  int n_possible;
  int n_beta;       /* Number of beta cutoffs */ 
  int n_ai_moves;
} search_context_s;

void do_search(state_s *state, search_result_s *res);
//void set_depth(int d);
/*
void stop_search(void);
extern int search_depth;
*/

#endif // SEARCH_H
