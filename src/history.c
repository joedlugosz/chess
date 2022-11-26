/*
 *  Position history stack and repetition checking
 */

#include "history.h"

#include <stdio.h>

#include "debug.h"
#include "search.h"

/* Push position data onto the top of the history stack */
void history_push(struct history *history, hash_t hash, struct move *move) {
  history->hash[history->index] = hash;
  history->is_breaking_move[history->index] = (move->result & (CAPTURED | PROMOTED)) ? 1 : 0;
  history->index++;
}

/* Pop position data from the top of history stack */
hash_t history_pop(struct history *history) {
  ASSERT(history->index >= 0);
  history->index--;
  return history->hash[history->index];
}

/* Clear the position history */
void history_clear(struct history *history) { memset(history, 0, sizeof(*history)); }

/* Check for whether a position has been encountered before, to satisfy three-
   or fivefold repetition rules. Search back every 2nd position from most recent
   to bottom of stack, keeping a count of the number of times a hash is found on
   the stack that matches the supplied hash. If a breaking move is encoutered
   which makes it impossible for the position to be repeated, there is no need
   to continue searching. Return true if there is are more repetitions than
   allowed.  */
int is_repeated_position(struct history *history, hash_t hash, int repetitions) {
  int index = history->index;
  int count = 0;
  while (index > 0) {
    index -= 2;
    if (history->hash[index] == hash) count++;
    if (history->is_breaking_move[index] == hash) break;
  }
  return (count >= repetitions);
}
