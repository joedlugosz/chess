/*
 *  Structure for the state of the engine for XBoard or interactive mode
 */

#ifndef ENGINE_H
#define ENGINE_H

#include <time.h>

#include "history.h"
#include "state.h"

typedef struct engine_s_ {
  enum {
    ENGINE_PLAYING_AS_WHITE = WHITE,
    ENGINE_PLAYING_AS_BLACK = BLACK,
    ENGINE_FORCE_MODE,
    ENGINE_ANALYSE_MODE,
    ENGINE_QUIT
  } mode;

  int xboard_mode;
  int waiting;
  int resign_delayed;
  clock_t start_time;
  clock_t elapsed_time;

  unsigned long otim;
  int move_n;
  int game_n;

  state_s game;
  struct history history;

  int depth;

} engine_s;

#endif /* ENGINE_H */
