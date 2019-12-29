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

typedef enum ai_status_e_ {
  NO_MOVE = 0,
  MOVED,
  STALEMATE,
  CHECKMATE
} ai_status_e;

typedef struct ai_result_s_ {
  ai_status_e status;
  score_t score;
  int n_searched;
  int n_possible;
  double cutoff;
} ai_result_s;

typedef struct notation_s_
{
  score_t score;
  pos_t from;
  pos_t to;
  player_e player;
  status_t captured : 1;
  status_t check : 1;
} notation_s;

/* move_s holds the game state as well as info about moves */
typedef struct move_s_ {
  pos_t from, to;
  score_t score;
  //  state_s state;
  struct move_s_ *next;
} move_s;

enum {
  SEARCH_DEPTH_MAX = 30,
  REPEAT_HISTORY_SIZE = 300,
  N_MOVES = 218
};

typedef struct search_s_ {
  int depth;
  int n_searched;
  int n_possible;
  int n_beta;
  int n_ai_moves;
  int running;
  pos_t found_from, found_to;
  notation_s search_history[SEARCH_DEPTH_MAX];
  notation_s repeat_history[REPEAT_HISTORY_SIZE];
} search_s;

void do_ai_move(state_s *state, ai_result_s *res);
void set_depth(int d);
/*
void stop_search(void);

extern int search_depth;
//extern const int player_factor[N_PLAYERS];
*/

#endif // SEARCH_H
