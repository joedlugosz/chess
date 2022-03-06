/* Portable command line parsing */

#ifndef CMDLINE_H
#define CMDLINE_H

/* Internal context for command line parser */
struct cmdline;

/* Callback to recieve an argument */
typedef int (*arg_fn)(struct cmdline *);

/* Argument definition provided in an array by client */
struct cmdline_def {
  char letter;
  char name[10];
  arg_fn fn;
  char description[100];
  char arg_description[100];
};

int cmdline_parse(const struct cmdline_def *defs, int argc, const char **argv);
const char *cmdline_get(struct cmdline *cmdline);
void cmdline_unget(struct cmdline *cmdline);
void cmdline_show(const struct cmdline_def *defs);

#endif  // CMDLINE_H
