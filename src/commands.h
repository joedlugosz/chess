/*
 *  Commands for XBoard interface and UI
 */

#ifndef COMMANDS_H
#define COMMANDS_H

struct engine_s_;
typedef void (*ui_fn)(struct engine_s_ *);

int accept_command(struct engine_s_ *, const char *);

#endif /* COMMANDS_H */
