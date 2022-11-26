/*
 *  UI and XBoard Interface
 */

#ifndef UI_H
#define UI_H

void enter_xboard_mode(struct struct engine_ *);
void init_engine(struct struct engine_ *engine);
void run_engine(struct struct engine_ *engine);
// void start_session_log(void);
int ui_no_piece_at_square(struct struct engine_ *, enum square);
int move_is_illegal(struct struct engine_ *, struct struct move_ *);

#endif /* UI_H */
