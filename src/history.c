/*
 *   Move history
 */

#include "history.h"

#include <stdio.h>

#include "debug.h"
#include "search.h"

/* Write a new move into the search history */
void write_search_history(search_job_s *job, int depth, move_s *move) {
  memcpy(&job->search_history[job->depth - depth], move, sizeof(*job->search_history));
}

/* Push position hash onto game history stack */
void history_push(struct history *history, hash_t hash) {
  history->hash[history->index] = hash;
  history->index++;
}

/* Pop position from the game histoy stack */
hash_t history_pop(struct history *history) {
  ASSERT(history->index >= 0);
  history->index--;
  return history->hash[history->index];
}

/* Simple check for whether a position has been encountered before */
int is_repeated_position(struct history *history, hash_t hash, int moves) {
  int index = history->index;
  while (moves--) {
    index -= 2;
    if (index < 0) break;
    if (history->hash[index] == hash) return 1;
  }
  return 0;
}
