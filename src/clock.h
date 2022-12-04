#ifndef CLOCK_H
#define CLOCK_H

#include "os.h"
#include "position.h"

struct clock {
  enum player turn;
  double time_control;
  int moves_per_session;
  double increment_seconds;
  double game_duration;
  double game_start;
  double period_start;
  double remaining[2];
  double last[2];
};

static inline void clock_start_game(struct clock *clock, enum player turn) {
  clock->turn = turn;
  clock->game_duration = clock->time_control;
  double time = time_now();
  clock->game_start = time;
  clock->period_start = time;
}

static inline void clock_start_turn(struct clock *clock, enum player turn) {
  clock->turn = turn;
  double time = time_now();
  clock->last[clock->turn] = time - clock->period_start;
  clock->remaining[clock->turn] -= clock->last[clock->turn];
  clock->period_start = time;
}

static inline void clock_reset_period(struct clock *clock) {
  clock->period_start = time_now();
}

#endif  // CLOCK_H
