/*
 *  Commands for XBoard interface and UI
 */

#ifndef COMMANDS_H
#define COMMANDS_H

struct struct engine;
typedef void (*ui_fn)(struct struct engine *);

int accept_command(struct struct engine *, const char *);

#endif /* COMMANDS_H */
