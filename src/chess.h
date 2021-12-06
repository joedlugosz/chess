/*
 *   4.0 Store piece totals on extra planes
 *   4.1 Windows supported
 */
#ifndef CHESS_H
#define CHESS_H
//#include "compiler.h"
//#include "compiler.h"
/* Configuration */
//#define YES 1
//#define NO 0
//#define CPU_64BIT 0
//#define ORDER_BINARY 0

/* Includes */
//#include <stdio.h>
//#include <time.h>
//#include <string.h>

/* 
 *  Basic definitions 
 */
/* Players */
typedef enum player_e_ {
  WHITE = 0, BLACK, N_PLAYERS
} player_e;

extern const char player_text[N_PLAYERS][6];


//enum {
  /* TODO: Is there a rule about this? */
//  GAME_MOVES_MAX = 200,
//};

/* */
//typedef unsigned long long hash_t;
//typedef unsigned char status_t;
//typedef int score_t;

//#include "lowlevel.h"
/* Game status */



#endif /* CHESS_H */
