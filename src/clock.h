#ifndef CLOCK_H
#define CLOCK_H

#include <math.h>

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
  int turns[2];
};

static inline void clock_start_game(struct clock *clock, enum player turn,
                                    double time_control, int turns) {
  clock->turn = turn;
  clock->time_control = time_control;
  clock->game_duration = time_control;
  clock->moves_per_session = turns;
  clock->remaining[WHITE] = time_control;
  clock->remaining[BLACK] = time_control;
  clock->turns[WHITE] = turns;
  clock->turns[BLACK] = turns;
  double time = time_now();
  clock->game_start = time;
  clock->period_start = time;
}

static inline void clock_start_turn(struct clock *clock, enum player turn) {
  double time = time_now();
  clock->last[clock->turn] = time - clock->period_start;
  clock->remaining[clock->turn] -= clock->last[clock->turn];
  clock->period_start = time;
  if (clock->turns[clock->turn] > 0)
    clock->turns[clock->turn]--;
  else
    clock->turns[clock->turn] = clock->moves_per_session;
  clock->turn = turn;
}

/* Set the player's remaining time.
 * If the remaining time increases significantly, start a new time control
 * period. */
static inline void clock_set_remaining(struct clock *clock, double remaining) {
  if (remaining > clock->remaining[clock->turn] + 1.0) {
    clock->turns[clock->turn] = clock->moves_per_session;
  }
  clock->remaining[clock->turn] = remaining;
}

static inline void clock_reset_period(struct clock *clock) {
  clock->period_start = time_now();
}

static inline double clock_get_time_budget(struct clock *clock,
                                           enum player turn) {
  printf("tc %lf mps %d rtm %lf rtn %d\n", clock->time_control,
         clock->moves_per_session, clock->remaining[turn], clock->turns[turn]);
  double target_average_time =
      clock->time_control / (double)clock->moves_per_session;
  double time_budget = clock->remaining[turn] / (double)clock->turns[turn];
  time_budget = fmax(time_budget, target_average_time / 3.0);
  time_budget = fmin(time_budget, target_average_time * 3.0);
  return time_budget;
}

#endif  // CLOCK_H
