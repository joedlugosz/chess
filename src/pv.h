/*
 *   Principal Variation History
 */

#ifndef PV_H
#define PV_H

#include <string.h>

#include "search.h"

struct struct move_;

/* PV history struct - a sized array of moves */
struct pv {
  int length;
  struct struct move_ moves[SEARCH_DEPTH_MAX];
};

/* Add a move to the principal variation history. This is called before returning
   up through the search tree upon an alpha update. It sets the PV history for the
   parent search node to be the updating move at index 0, followed by the PV history
   for this node from index 1 onwards. In this way, the history is built up, with
   the root move at index 0 */
static inline void pv_add(struct pv *parent, struct pv *child, struct struct move_ *move) {
  memcpy(&parent->moves[0], move, sizeof(parent->moves[0]));
  memcpy(&parent->moves[1], &child->moves[0], child->length * sizeof(parent->moves[0]));
  parent->length = child->length + 1;
}

#endif  // PV_H
