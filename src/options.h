#ifndef OPTIONS_H
#define OPTIONS_H

#include "commands.h"

enum { NAME_LENGTH = 50 };

/* Single combo box selection value */
struct combo_val {
  char name[NAME_LENGTH];
  int value;
};

/* Set of values for a combo box */
struct combo {
  int n_vals;
  const struct combo_val *const vals;
};

/* Option types defined by XBoard protocol */
enum option_type {
  BOOL_OPT = 0, /* Check box */
  SPIN_OPT,     /* Spin control */
  INT_OPT,   /* Integer - presented as a text input to XBoard then validated */
  TEXT_OPT,  /* Text control */
  CMD_OPT,   /* Button */
  COMBO_OPT, /* Combo box */
  N_OPTION_T
};

/* Tagged union for an option */
struct option {
  char name[NAME_LENGTH];
  enum option_type type;

  union {
    int *integer; /* type == BOOL_OPT || SPIN_OPT || INT_OPT - ptr to int value
                     type == COMBO_OPT - ptr to int index into combo_vals */
    char *text;   /* type == TEXT_OPT - ptr to start of text */
    ui_fn function; /* type == CMD_OPT - ptr to command function */
  } value;

  /* type == SPIN_OPT - limit values for UI spin control */
  int min;
  int max;

  /* type == COMBO_OPT - array of options indexed by *value.integer */
  const struct combo *combo_vals;
};

/* Set of options for a module */
struct options {
  int n_opts;
  const struct option *const opts;
};

void list_features(void);
void feature_accepted(const char *name);
void list_options(void);
int set_option(struct engine *e, const char *name);

#endif /* OPTIONS_H */
