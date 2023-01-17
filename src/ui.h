/*
 *  UI and XBoard Interface
 */

#ifndef UI_H
#define UI_H

struct engine;

void enter_xboard_mode(struct engine *engine);
void init_engine(struct engine *engine);
void run_engine(struct engine *engine);
int ui_no_piece_at_square(const struct engine *engine, enum square square);
int ui_move_is_illegal(const struct engine *engine, struct move *move);
void reset_time_control(struct engine *engine);

#endif /* UI_H */
