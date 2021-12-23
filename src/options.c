/*
 *    Options and features interface to XBoard
 */

#include <ctype.h>
#include <stdint.h>

#include "commands.h"
#include "engine.h"
#include "io.h"
#include "log.h"
#include "search.h"

/* Options */

/* Arrays of options are declared in other modules */
extern const options_s engine_options;
extern const options_s search_opts;
extern const options_s eval_opts;
extern const options_s log_opts;

const options_s *const module_opts[] = {&search_opts, &eval_opts, &engine_options, &log_opts};
enum { N_MODULES = sizeof(module_opts) / sizeof(module_opts[0]) };

/* Names passed to XBoard describing option types - see definition of option_type_e */
const char option_controls[N_OPTION_T][10] = {"check",  "spin",   "string",
                                              "string", "button", "combo"};

/* List available options in response to a `protover` request from XBoard */
void list_options(void) {
  /* For each program module */
  for (int i = 0; i < N_MODULES; i++) {
    const options_s *const mod = module_opts[i];
    /* For each option within the module */
    for (int j = 0; j < mod->n_opts; j++) {
      const option_s *opt = &mod->opts[j];

      /* Describe option to XBoard */
      printf("feature option=\"%s -%s", opt->name, option_controls[opt->type]);

      switch (opt->type) {
        case SPIN_OPT:
          /* e.g. `feature option="foo -spin 50 1 100"\n` */
          printf(" %d %d %d", *opt->value.integer, opt->min, opt->max);
          break;
        case BOOL_OPT:
          /* e.g. `feature option="foo -check 0"\n` */
        case INT_OPT:
          /* e.g. `feature option="foo -string 100"\n` */
          printf(" %d", *opt->value.integer);
          break;
        case TEXT_OPT:
          /* e.g. `feature option="foo -string abcde"\n` */
          printf(" %s", opt->value.text);
          break;
        case CMD_OPT:
          /* e.g. `feature option="foo -button"\n` */
          break;
        case COMBO_OPT: {
          /* e.g. `feature option="foo -combo *opt1///opt2///opt3"\n` */
          for (int k = 0; k < opt->combo_vals->n_vals; k++) {
            const combo_val_s *val = &opt->combo_vals->vals[k];
            printf(" %s%s%s", (k == 0) ? "" : "/// ", ((*opt->value.integer) == k) ? "*" : "",
                   val->name);
          }
        }
        default:
          break;
      }
      printf("\"\n");
    }
  }
}

void read_option_text(char *buf) {
  char in = 0;
  /* Skip any whitespace at the start */
  while (isspace(in = fgetc(stdin)))
    ;
  /* Read option text into buf up to newline */
  char *ptr = buf;
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
  const char *val_txt = "";
  /* Go through all options for all modules to look for one that matches name */
  for (i = 0; i < N_MODULES && !err; i++) {
    const options_s *const mod = module_opts[i];
    for (j = 0; j < mod->n_opts && !err; j++) {
      const option_s *opt = &mod->opts[j];
      /* If a match is found... */
      if (strcmp(name, opt->name) == 0) {
        /* Read arguments */
        switch (opt->type) {
          default:
            break;
          case CMD_OPT:
            /* No arguments */
            break;
          case SPIN_OPT:
          case BOOL_OPT:
          case INT_OPT:
          case TEXT_OPT:
          case COMBO_OPT:
            /* Arguments are passed as a string terminated by newline */
            val_txt = get_delim('\n');
            break;
        }

        /* Interpretation */
        switch (opt->type) {
          default:
            break;
          case SPIN_OPT:
          case BOOL_OPT:
          case INT_OPT:
            /* Integer  */
            if (sscanf(val_txt, "%d", &val) != 1) {
              err = 1;
            }
            break;
          case TEXT_OPT:
            /* Read text into opt->value */
            strcpy(opt->value.text, val_txt);
            printf("%s", opt->value.text);
            break;
          case CMD_OPT:
            /* Call the function pointed to by opt->value */
            (opt->value.function)(e);
            break;
        }

        if (err) continue;

        /* Validation */
        switch (opt->type) {
          default:
            break;
          case TEXT_OPT:
            /* Text is not validated */
            break;
          case SPIN_OPT:
            if (val > opt->max || val < opt->min) {
              return 1;
            } else {
              *opt->value.integer = val;
              break;
            }
          case BOOL_OPT:
            if (val > 1 || val < 0) {
              return 1;
            } else {
              *opt->value.integer = val;
            }
            break;
          case INT_OPT:
            *opt->value.integer = val;
            break;
          case COMBO_OPT: {
            /* Look up option value by name */
            int found_val = 0;
            for (int k = 0; k < opt->combo_vals->n_vals; k++) {
              if (strcmp(opt->combo_vals->vals[k].name, val_txt) == 0) {
                *opt->value.integer = k;
                found_val = 1;
                break;
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
  if (!found) return 1;
  return 0;
}

/* Features */

typedef enum feature_type_e_ { INT_FEAT = 0, TEXT_FEAT } feature_type_e;

typedef struct feature_s_ {
  char name[NAME_LENGTH];
  feature_type_e type;
  int int_val;
  const char text_val[NAME_LENGTH];
} feature_s;

const feature_s features[] = {{"myname", TEXT_FEAT, 0, "JoeChess 0.1"},
                              {"setboard", INT_FEAT, 0, ""},
                              {"name", INT_FEAT, 0, ""},
                              {"ping", INT_FEAT, 0, ""},
                              {"done", INT_FEAT, 0, ""},
                              {"variants", TEXT_FEAT, 0, "normal"},
                              {"done", INT_FEAT, 1, ""}};

enum { N_FEATURES = sizeof(features) / sizeof(features[0]) };

/* Feature accepted - currently these values are set but not used */
int feature_acc[N_FEATURES];

/* List features in response to a `protover` request from XBoard */
void list_features(void) {
  for (int i = 0; i < N_FEATURES; i++) {
    printf("feature %s=", features[i].name);
    switch (features[i].type) {
      case INT_FEAT:
        /* e.g. `feature foo=1\n` */
        printf("%d", features[i].int_val);
        break;
      case TEXT_FEAT:
        /* e.g. `feature variants="normal"\n` */
        printf("\"%s\"", features[i].text_val);
        break;
    }
    printf("\n");
  }
}

/* XBoard has notified that a named feature has been accepted */
void feature_accepted(const char *name) {
  for (int i = 0; i < N_FEATURES; i++) {
    if (strcmp(name, features[i].name) == 0) {
      feature_acc[i] = 1;
      PRINT_LOG(&xboard_log, "%s", buf);
      break;
    }
  }
}
