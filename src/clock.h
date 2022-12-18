/*
 * Time management
 */

#ifndef CLOCK_H
#define CLOCK_H

#include <math.h>

#include "os.h"
#include "position.h"

enum time_ctrl_mode {
  TIME_CTRL_CLASSICAL,
  TIME_CTRL_INCREMENTAL,
  TIME_CTRL_FIXED
};

struct clock {
  enum time_ctrl_mode mode;
  /* The duration of each time control period */
  double time_control;
  /* The number of moves in each time control period */
  int moves_per_session;
  /* Fixed increment mode */
  double increment_seconds;

  /* The monotonic time that the current move started, in seconds */
  double move_start;
  /* Time remaining in the current time control period, in seconds */
  double time_remaining[2];
  /* Time that the player took making their last move, in seconds */
  double last_move_time[2];
  /* Moves remaining in the current time control period */
  int moves_remaining[2];
};

/* Reset the start time of the current move */
static inline void clock_reset_period(struct clock *clock) {
  clock->move_start = time_now();
}

/* Initialise time control for a new game */
static inline void clock_start_game(struct clock *clock) {
  clock->time_remaining[WHITE] = clock->time_control;
  clock->time_remaining[BLACK] = clock->time_control;
  clock->moves_remaining[WHITE] = clock->moves_per_session;
  clock->moves_remaining[BLACK] = clock->moves_per_session;
  clock_reset_period(clock);
}

/* Called at the end of a player's turn.  Calculate */
static inline void clock_end_turn(struct clock *clock, enum player turn) {
  double time = time_now();
  clock->last_move_time[turn] = time - clock->move_start;
  clock->move_start = time;
  switch (clock->mode) {
    case TIME_CTRL_CLASSICAL:
      clock->time_remaining[turn] -= clock->last_move_time[turn];
      if (clock->moves_remaining[turn] > 0)
        clock->moves_remaining[turn]--;
      else
        clock->moves_remaining[turn] = clock->moves_per_session;
      break;
    case TIME_CTRL_INCREMENTAL:
      if (clock->moves_remaining[turn] > 0) clock->moves_remaining[turn]--;
      clock->time_remaining[turn] -= clock->last_move_time[turn];
      clock->time_remaining[turn] += clock->increment_seconds;
      break;
    case TIME_CTRL_FIXED:
    default:
      clock->time_remaining[turn] = clock->increment_seconds;
      break;
  }
}

/* Set the player's remaining time.  If the remaining time increases
 * significantly, start a new time control period. */
static inline void clock_set_remaining(struct clock *clock,
                                       double time_remaining,
                                       enum player turn) {
  if (time_remaining > clock->time_remaining[turn] + 1.0)
    clock->moves_remaining[turn] = clock->moves_per_session;
  clock->time_remaining[turn] = time_remaining;
}

/* Get the time budget to make one move. */
static inline double clock_get_time_budget(struct clock *clock,
                                           enum player turn) {
  switch (clock->mode) {
    case TIME_CTRL_CLASSICAL: {
      double target_average_time =
          clock->time_control / (double)clock->moves_per_session;
      double time_budget =
          clock->time_remaining[turn] / (double)clock->moves_remaining[turn];
      time_budget = fmax(time_budget, target_average_time / 3.0);
      time_budget = fmin(time_budget, target_average_time * 3.0);
      printf("cl tc %lf mps %d tr %lf mr %d stb %lf\n", clock->time_control,
             clock->moves_per_session, clock->time_remaining[turn],
             clock->moves_remaining[turn], time_budget);

      return time_budget;
    }
    case TIME_CTRL_INCREMENTAL:
      return fmin(clock->time_remaining[turn],
                  clock->time_control / 40.0 + clock->time_remaining[turn]);
    case TIME_CTRL_FIXED:
    default:
      return clock->increment_seconds;
  }
}

static inline double clock_get_time_margin(struct clock *clock) {
  switch (clock->mode) {
    case TIME_CTRL_CLASSICAL:
      return 1.0;
    case TIME_CTRL_INCREMENTAL:
      return 0.5;
    default:
      return 0.0;
  }
}

#endif  // CLOCK_H
