#ifndef OPTIONS_H
#define OPTIONS_H

#include "commands.h"

enum { NAME_LENGTH = 50 };

/* Single combo box selection value */
typedef struct combo_val_s_ {
  char name[NAME_LENGTH];
  int value;
} combo_val_s;

/* Set of values for a combo box */
typedef struct combo_s_ {
  int n_vals;
  const combo_val_s *const vals;
} combo_s;

/* Option types defined by XBoard protocol */
typedef enum option_type_e_ {
  BOOL_OPT = 0, /* Check box */
  SPIN_OPT,     /* Spin control */
  INT_OPT,      /* Integer - presented as a text input to XBoard then validated */
  TEXT_OPT,     /* Text control */
  CMD_OPT,      /* Button */
  COMBO_OPT,    /* Combo box */
  N_OPTION_T
} option_type_e;

/* Tagged union for an option */
typedef struct option_s_ {
  char name[NAME_LENGTH];
  option_type_e type;

  union {
    int *integer;   /* type == BOOL_OPT || SPIN_OPT || INT_OPT - ptr to int value
                       type == COMBO_OPT - ptr to int index into combo_vals */
    char *text;     /* type == TEXT_OPT - ptr to start of text */
    ui_fn function; /* type == CMD_OPT - ptr to command function */
  } value;

  /* type == SPIN_OPT - limit values for UI spin control */
  int min;
  int max;

  /* type == COMBO_OPT - array of options indexed by *value.integer */
  const combo_s *combo_vals;

} option_s;

/* Set of options for a module */
typedef struct options_s_ {
  int n_opts;
  const option_s *const opts;
} options_s;

void list_features(void);
void feature_accepted(const char *name);
void list_options(void);
int set_option(struct engine_s_ *e, const char *name);

#endif /* OPTIONS_H */
