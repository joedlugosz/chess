#ifndef UI_H
#define UI_H

#include "compiler.h"

int no_piece_at_pos(struct engine_s_ *, pos_t);
int move_is_illegal(struct engine_s_ *, struct move_s_ *);

#endif /* UI_H */
