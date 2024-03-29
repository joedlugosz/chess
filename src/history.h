/*
 *  Position history stack and repetition checking
 */

#ifndef HISTORY_H
#define HISTORY_H

#include "position.h"
#include "search.h"

/* Position history stack */
struct history {
  int index;
  int is_breaking_move[REPEAT_HISTORY_SIZE];
  hash_t hash[REPEAT_HISTORY_SIZE];
};

void history_push(struct history *history, hash_t hash, struct move *move);
hash_t history_pop(struct history *history);
void history_clear(struct history *history);
int is_repeated_position(const struct history *history, hash_t hash, int moves);

#endif /* HISTORY_H */
