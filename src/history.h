/*
 *   Move history
 */

#ifndef HISTORY_H
#define HISTORY_H

#include "search.h"
#include "state.h"

struct history {
  int index;
  int breaking[REPEAT_HISTORY_SIZE];
  hash_t hash[REPEAT_HISTORY_SIZE];
};

void history_push(struct history *history, hash_t hash, struct move_s_ *move);
hash_t history_pop(struct history *history);
void history_clear(struct history *history);
int is_repeated_position(struct history *history, hash_t hash, int moves);

#endif /* HISTORY_H */
