/*
 *   Move history
 */

#include "history.h"
#include "search.h"

void write_search_history(search_job_s *job, int depth, move_s *move)
{
  memcpy(&job->search_history[depth], move, sizeof(*job->search_history));
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
  }
  return 0;
}
