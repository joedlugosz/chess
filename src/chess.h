/*
 *   4.0 Store piece totals on extra planes
 *   4.1 Windows supported
 */
#ifndef CHESS_H
#define CHESS_H
#include "language.h"

/* Configuration */
#define YES 1
#define NO 0
#define CPU_64BIT 0
#define ORDER_BINARY 0

/* Includes */
#include <stdio.h>
#include <time.h>
#include <string.h>

/* 
 *  Basic definitions 
 */
/* Players */
typedef enum player_e_ {
  WHITE = 0, BLACK, N_PLAYERS
} player_e;

typedef unsigned long long plane_t;


enum {
  /* TODO: Is there a rule about this? */
  GAME_MOVES_MAX = 200,
};

/* */
typedef unsigned long long hash_t;
/* Game status */
typedef unsigned char status_t;
//typedef int score_t;

//#include "lowlevel.h"



/* Game */


/* Evaluation */

/* Searching */
/*

*/

/*
 * Functions 
 */

/* game.c */
//pos_t random_pos(void);

/* search.c */
/* eval.c */

/* log.c */

extern const char player_text[N_PLAYERS][6];

//void xboard_thought(FILE *f, int depth, score_t score, clock_t time, int nodes);
//void debug_thought(FILE *f, search_s *ctx, int depth, score_t score, score_t alpha, score_t beta);


#endif /* CHESS_H */
