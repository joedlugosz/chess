/*
 *  Commands for XBoard interface and UI
 */

#ifndef COMMANDS_H
#define COMMANDS_H

struct engine;

typedef void (*ui_fn)(struct engine *);

int accept_command(struct engine *, const char *in);

#endif /* COMMANDS_H */
