/*
 *   Move history
 */

#include "history.h"

#include "search.h"

/* Write a new move into the search history */
void write_search_history(search_job_s *job, int depth, move_s *move) {
  memcpy(&job->search_history[depth], move, sizeof(*job->search_history));
}

/* Simple check for if a move has been made before */
/* TODO: Transposition table */
int is_repeated_move(search_job_s *job, int depth, move_s *move) {
  if (depth > 1 && move->to == job->search_history[depth - 2].from &&
      move->from == job->search_history[depth - 2].to) {
    return 1;
  }
  return 0;
}
