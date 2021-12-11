#ifndef OPTIONS_H
#define OPTIONS_H

struct engine_s_;

enum {
  NAME_LENGTH = 50
};

typedef struct combo_val_s_ {
  char name[NAME_LENGTH];
  void *value;
} combo_val_s;

typedef struct combo_s_ {
  int n_vals;
  const combo_val_s *const vals;
} combo_s;

typedef enum option_type_e_ {
  BOOL_OPT = 0, SPIN_OPT, INT_OPT, TEXT_OPT, CMD_OPT, COMBO_OPT, N_OPTION_T
} option_type_e;

typedef struct option_s_ {
  char name[NAME_LENGTH];
  option_type_e type;
  void *value;
  int min;
  int max;
  const combo_s *combo_vals;
} option_s;

typedef struct options_s_ {
  int n_opts;
  const option_s *const opts;
} options_s;

void list_features(void);
void feature_accepted(const char *name);
void list_options(void);
int set_option(struct engine_s_ *e, const char *name);

#endif /* OPTIONS_H */
