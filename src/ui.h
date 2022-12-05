/*
 *  UI and XBoard Interface
 */

#ifndef UI_H
#define UI_H

struct engine;

void enter_xboard_mode(struct engine *engine);
void init_engine(struct engine *engine);
void run_engine(struct engine *engine);
// void start_session_log(void);
int ui_no_piece_at_square(const struct engine *engine, enum square square);
int ui_move_is_illegal(const struct engine *engine, struct move *move);

extern int search_depth;
extern int time_control;
extern int time_control_moves;
extern int time_increment;
#endif /* UI_H */
