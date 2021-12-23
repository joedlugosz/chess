/*
 *  Program entry point and user interface.
 */

#ifndef UI_H
#define UI_H

int no_piece_at_square(struct engine_s_ *, square_e);
int move_is_illegal(struct engine_s_ *, struct move_s_ *);

#endif /* UI_H */
