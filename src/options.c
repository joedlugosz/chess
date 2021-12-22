#include <ctype.h>
#include <stdint.h>

#include "commands.h"
#include "engine.h"
#include "io.h"
#include "log.h"
#include "search.h"

/* Options */
enum {
  N_MODULES = 4,
};

extern const options_s engine_options;
extern const options_s search_opts;
extern const options_s eval_opts;
extern const options_s log_opts;

const options_s *const module_opts[N_MODULES] = {&search_opts, &eval_opts, &engine_options,
                                                 &log_opts};

/* See chess.h - option_type_e */
const char option_controls[N_OPTION_T][10] = {"check",  "spin",   "string",
                                              "string", "button", "combo"};

void list_options(void) {
  int i, j;
  for (i = 0; i < N_MODULES; i++) {
    const options_s *const mod = module_opts[i];
    for (j = 0; j < mod->n_opts; j++) {
      const option_s *opt = mod->opts + j;
      printf("feature option=\"%s -%s", opt->name, option_controls[opt->type]);
      switch (opt->type) {
        case SPIN_OPT:
          printf(" %d %d %d", *(int *)(opt->value), opt->min, opt->max);
          break;
        case BOOL_OPT:
        case INT_OPT:
          printf(" %d", *(int *)(opt->value));
          break;
        case TEXT_OPT:
          printf(" %s", (char *)(opt->value));
          break;
        case CMD_OPT:
          break;
        case COMBO_OPT: {
          int k;
          printf(" ");
          for (k = 0; k < opt->combo_vals->n_vals; k++) {
            const combo_val_s *val = opt->combo_vals->vals + k;
            printf("%s%s %s", ((*((int *)(opt->value)) == k) ? "*" : ""), val->name,
                   (k < opt->combo_vals->n_vals - 1) ? "/// " : "");
          }
        }
        case N_OPTION_T:
          break;
      }
      printf("\"\n");
    }
  }
}

void read_option_text(char *buf) {
  char in, *ptr;

  /* Skip any whitespace at the start */
  while (isspace(in = fgetc(stdin)))
    ;
  /* Read option text into buf up to newline */
  ptr = buf;
  while (in != '\n') {
    *ptr++ = in;
    in = fgetc(stdin);
  }
  /* Trim any whitespace at the end */
  *ptr-- = 0;
  while (isspace(*ptr)) {
    *ptr-- = 0;
  }
}

int set_option(engine_s *e, const char *name) {
  int found = 0;
  int i, j;
  int val;
  int err = 0;
  // char buf[NAME_LENGTH];
  const char *val_txt = "";
  /* Go through all options for all modules to look for one that matches name */
  for (i = 0; i < N_MODULES; i++) {
    const options_s *const mod = module_opts[i];
    for (j = 0; j < mod->n_opts; j++) {
      const option_s *opt = mod->opts + j;
      /* If a match is found */
      if (strcmp(name, opt->name) == 0) {
        /* Try to read in arguments depending on option type */
        switch (opt->type) {
          default:
            break;
          case SPIN_OPT:
          case BOOL_OPT:
          case INT_OPT:
          case TEXT_OPT:
          case COMBO_OPT:
            val_txt = get_delim('\n');
            break;
        }
        switch (opt->type) {
          default:
            break;
          case SPIN_OPT:
          case BOOL_OPT:
          case INT_OPT:

            /* Try to read a number, opt->value will be
               updated later after checking  */
            if (sscanf(val_txt, "%d", &val) != 1) {
              err = 1;
            }
            break;
          case TEXT_OPT:
            /* Read text into opt->value */
            strcpy((char *)opt->value, val_txt);
            printf("%s", (char *)(opt->value));
            break;
          case CMD_OPT:
            /* Call the function pointed to by opt->value */
            (*(ui_fn)(opt->value))(e);
            break;
        }
        /* Try to interpret and store the arguments depending on type */
        if (!err) {
          switch (opt->type) {
            default:
              break;
            case SPIN_OPT:
              if (val > opt->max || val < opt->min) {
                return 1;
              } else {
                *(int *)opt->value = val;
                break;
              }
            case BOOL_OPT:
              if (val > 1 || val < 0) {
                return 1;
              } else {
                *(int *)opt->value = val;
              }
              break;
            case INT_OPT:
              *(int *)opt->value = val;
              break;
            case COMBO_OPT: {
              int found_val = 0;
              int i;
              for (i = 0; i < opt->combo_vals->n_vals; i++) {
                if (strcmp(opt->combo_vals->vals[i].name, val_txt) == 0) {
                  *((intptr_t *)(opt->value)) = (intptr_t)(opt->combo_vals->vals[i].value);
                  found_val = 1;
                }
              }
              if (!found_val) return 1;
              break;
            }
          }
          found = 1;
        }
      }
    }
  }
  if (!found) return 1;
  return 0;
}

/* Features */

enum {
  N_FEATURES = 7,
};

typedef enum feature_type_e_ { INT_FEAT = 0, TEXT_FEAT } feature_type_e;

typedef struct feature_s_ {
  char name[NAME_LENGTH];
  feature_type_e type;
  int int_val;
  const char text_val[NAME_LENGTH];
} feature_s;

const feature_s features[N_FEATURES] = {{"myname", TEXT_FEAT, 0, "JoeChess 0.1"},
                                        {"setboard", INT_FEAT, 0, ""},
                                        {"name", INT_FEAT, 0, ""},
                                        {"ping", INT_FEAT, 0, ""},
                                        {"done", INT_FEAT, 0, ""},
                                        {"variants", TEXT_FEAT, 0, "normal"},
                                        {"done", INT_FEAT, 1, ""}};

int feature_acc[N_FEATURES];

void list_features(void) {
  int i;
  for (i = 0; i < N_FEATURES; i++) {
    printf("feature %s=", features[i].name);
    switch (features[i].type) {
      case INT_FEAT:
        printf("%d", features[i].int_val);
        break;
      case TEXT_FEAT:
        printf("\"%s\"", features[i].text_val);
        break;
    }
    printf("\n");
  }
}

void feature_accepted(const char *buf) {
  int i;
  for (i = 0; i < N_FEATURES; i++) {
    if (strcmp(buf, features[i].name) == 0) {
      feature_acc[i] = 1;
      PRINT_LOG(&xboard_log, "%s", buf);
      break;
    }
  }
}
