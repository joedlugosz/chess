/*
 *   Move history
 */

#include "history.h"
#include "search.h"

void write_move_history(search_job_s *job, int depth, move_s *move,
  player_e to_move, score_t score)
{
  job->search_history[depth].from = move->from;
  job->search_history[depth].to = move->to;
  job->search_history[depth].player = to_move;
  job->search_history[depth].score = score;
}

int is_repeated_move(search_job_s *job, int depth, move_s *move)
{
  /* Super simple checking for moving back to the square you've
     just come from */
  /* TODO: Transposition table */
  if (depth > 1) {
    if (move->to == job->search_history[depth-2].from
	&& move->from == job->search_history[depth-2].to) {
      return 1;
    }
  } else if (job->n_ai_moves > 0) {
    if(move->to == job->repeat_history[job->n_ai_moves-1].from
       && move->from == job->repeat_history[job->n_ai_moves-1].to) {
      return 1;
    }
  }
  return 0;
}
