/*
 *   Position history
 */

#include "history.h"

#include <stdio.h>

#include "debug.h"
#include "search.h"

/* Push position data onto the history stack */
void history_push(struct history *history, hash_t hash, struct move_s_ *move) {
  history->hash[history->index] = hash;
  history->breaking[history->index] = (move->result & (CAPTURED | PROMOTED)) ? 1 : 0;
  history->index++;
}

/* Pop position from the history stack */
hash_t history_pop(struct history *history) {
  ASSERT(history->index >= 0);
  history->index--;
  return history->hash[history->index];
}

/* Clear the position history */
void history_clear(struct history *history) {
  memset(history, 0, sizeof(*history));
}

/* Check for whether a position has been encountered before, to satisfy three-
   or fivefold repetition rules. */
int is_repeated_position(struct history *history, hash_t hash, int repetitions) {

  /* Search back every 2nd position from top of stack to bottom of stack,
     keeping a count of the number of times a hash is found on the stack that
     matches the supplied hash. If a breaking move is encoutered which makes it
     impossible for the position to be repeated, there is no need to continue
     searching */
  int index = history->index;
  int count = 0;
  while (index > 0) {
    index -= 2;
    if (history->hash[index] == hash) count++;
    if (history->breaking[index] == hash) break;
  }

  /* Return true if there is are more repetitions than allowed  */
  return (count >= repetitions);
}
