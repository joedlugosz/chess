/*
 *  Program entry point and user interface.
 */

#ifndef UI_H
#define UI_H

void enter_xboard_mode(struct engine_s_ *);
void init_engine(struct engine_s_ *engine);
void run_engine(struct engine_s_ *engine);
// void start_session_log(void);
int ui_no_piece_at_square(struct engine_s_ *, square_e);
int move_is_illegal(struct engine_s_ *, struct move_s_ *);

#endif /* UI_H */
