/*
 *  Structure for the position of the engine for XBoard or interactive mode
 */

#ifndef ENGINE_H
#define ENGINE_H

#include <time.h>

#include "history.h"
#include "position.h"

struct engine {
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
  //  int move_n;
  int game_n;

  struct position game;
  struct history history;

  int depth;
};

#endif /* ENGINE_H */
