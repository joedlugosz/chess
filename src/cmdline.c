/* Portable command line parsing */

#include "cmdline.h"

#include <stdio.h>
#include <string.h>

/* Context struct for parsing */
struct cmdline {
  const char **argv;
  int argc;
  const struct cmdline_def *defs;
  int next;
};

/* Initialise a parsing job */
static void cmdline_init(struct cmdline *args, const struct cmdline_def *defs, int argc,
                         const char **argv) {
  args->argc = argc;
  args->argv = argv;
  args->defs = defs;
  args->next = 1;
}

/* Get the next command line argument */
const char *cmdline_get(struct cmdline *args) {
  if (args->next == args->argc) return 0;
  return args->argv[args->next++];
}

/* Unget a command line argument */
void cmdline_unget(struct cmdline *args) {
  if (args->next == 1) return;
  args->next--;
}

/* Print all command line argument definitions */
void cmdline_show(const struct cmdline_def *defs) {
  int index = 0;
  const struct cmdline_def *arg_def = &defs[index];
  while (arg_def->fn) {
    char names[100];
    names[0] = 0;
    if (arg_def->letter == 0 && arg_def->name[0] == 0) {
      sprintf(names, "%s", "");
    } else {
      if (arg_def->letter) {
        char buf[10];
        sprintf(buf, "-%c ", arg_def->letter);
        strcat(names, buf);
      }
      if (arg_def->name[0]) {
        char buf[100];
        sprintf(buf, "--%s ", arg_def->name);
        strcat(names, buf);
      }
    }
    char buf[200];
    sprintf(buf, "%s %s", names, arg_def->arg_description);
    printf("     %-20s %s\n", buf, arg_def->description);
    arg_def = &defs[++index];
  }
}

/* Find a function by long name */
static arg_fn cmdline_find_by_name(struct cmdline *args, const char *name) {
  int index = 0;
  const struct cmdline_def *arg_def = &args->defs[index];
  while (arg_def->fn) {
    if (strcmp(arg_def->name, name) == 0) {
      return arg_def->fn;
    }
    arg_def = &args->defs[++index];
  }
  return 0;
}

/* Find a function by letter */
static arg_fn cmdline_find_by_letter(struct cmdline *args, char name) {
  int index = 0;
  const struct cmdline_def *arg_def = &args->defs[index];
  while (arg_def->fn) {
    if (arg_def->letter == name) {
      return arg_def->fn;
    }
    arg_def = &args->defs[++index];
  }
  return 0;
}

/* Parse supplied command line arguments according to supplied definitions */
int cmdline_parse(const struct cmdline_def *defs, int argc, const char **argv) {
  struct cmdline args;
  cmdline_init(&args, defs, argc, argv);
  const char *arg = cmdline_get(&args);
  while (arg) {
    arg_fn fn = 0;
    if (arg[0] == 0) return 1;
    if (arg[0] == '-') {
      if (arg[1] == 0) return 1;
      if (arg[1] == '-') {
        if (arg[2] == 0) return 1;
        fn = cmdline_find_by_name(&args, &arg[2]);
      } else {
        fn = cmdline_find_by_letter(&args, arg[1]);
      }
    } else {
      cmdline_unget(&args);
      fn = cmdline_find_by_letter(&args, 0);
    }
    if (!fn) return 1;
    if ((*fn)(&args)) return 1;
    arg = cmdline_get(&args);
  }
  return 0;
}
