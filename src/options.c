/*
 * Options and features interface to XBoard
 */

#include "options.h"

#include <ctype.h>
#include <stdint.h>
#include <stdio.h>

#include "buildinfo/buildinfo.h"
#include "commands.h"
#include "debug.h"
#include "engine.h"
#include "io.h"
#include "search.h"

/*
 * Options
 */

/*
 * Arrays of options are declared in other modules
 */
extern const struct options eval_opts;
extern const struct options ui_opts;

/* Array of options from each module */
const struct options *const module_opts[] = {&ui_opts, &eval_opts};
enum { N_MODULES = sizeof(module_opts) / sizeof(module_opts[0]) };

/* Names which are passed to XBoard describing option types - see definition of
 * `enum option_type` */
const char option_controls[N_OPTION_T][10] = {"check",  "spin",   "string",
                                              "string", "button", "combo"};

/* List the available options in response to a `protover` request from XBoard */
void list_options(void) {
  /* For each program module and each option within the module, describe the
   * option to XBoard. */
  for (int i = 0; i < N_MODULES; i++) {
    const struct options *const mod = module_opts[i];
    for (int j = 0; j < mod->n_opts; j++) {
      const struct option *opt = &mod->opts[j];
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
            const struct combo_val *val = &opt->combo_vals->vals[k];
            printf(" %s%s%s", (k == 0) ? "" : "/// ",
                   ((*opt->value.integer) == k) ? "*" : "", val->name);
          }
        }
        default:
          break;
      }
      printf("\"\n");
    }
  }
}

/* Get arguments for an option, delimited by \n */
static inline const char *get_option_args(const struct option *opt) {
  switch (opt->type) {
    case CMD_OPT:
    default:
      /* No arguments */
      return "";
    case SPIN_OPT:
    case BOOL_OPT:
    case INT_OPT:
    case TEXT_OPT:
    case COMBO_OPT:
      return get_delim('\n');
  }
}

/* Interpret the arguments in `val_txt` for an `option` request from XBoard. */
static inline int interpret_option_args(const struct option *opt,
                                        struct engine *engine,
                                        const char *val_txt,
                                        int *val /* in/out */) {
  *val = 0;
  switch (opt->type) {
    default:
      break;
    case SPIN_OPT:
    case BOOL_OPT:
    case INT_OPT:
      if (sscanf(val_txt, "%d", val) != 1) return 1;
      break;
    case TEXT_OPT:
      strcpy(opt->value.text, val_txt);
      break;
    case CMD_OPT:
      (opt->value.function)(engine);
      break;
  }
  return 0;
}

/* Validate the arguments and store in opt->value.  For COMBO_OPT, resolve the
 * name in `val_txt` to an index value. */
static inline int validate_option_args(const struct option *opt,
                                       const char *val_txt, int val) {
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
  return 0;
}

/* Set an option in response to an `option` request from XBoard */
int set_option(struct engine *engine, const char *name) {
  /*
   * Go through all options for all modules to look for one that matches `name`.
   * If a match is found, read the arguments as a string terminated by newline.
   * Interpret the arguments as string, int, function call, etc. then validate
   * as necessary.  Handle validation errors by ignoring the option request and
   * proceeding to the next one.
   */
  int found = 0;
  int err = 0;
  for (int i = 0; i < N_MODULES && !err; i++) {
    const struct options *const mod = module_opts[i];
    for (int j = 0; j < mod->n_opts && !err; j++) {
      const struct option *opt = &mod->opts[j];
      if (strcmp(name, opt->name) == 0) {
        const char *val_txt = get_option_args(opt);
        int val = 0;
        if (interpret_option_args(opt, engine, val_txt, &val)) continue;
        if (validate_option_args(opt, val_txt, val)) continue;
        found = 1;
      }
    }
  }
  if (!found) return 1;
  return 0;
}

/*
 * Features interface to XBoard
 *
 * In response to a `protover` request, the engine describes its available
 * features and characteristics to XBoard, if they are different from expected
 * defaults.  Then XBoard has the option to accept or reject them.  Features are
 * reported as either text or integer, e.g.:
 *
 * feature foo=1\n
 * feature variants="normal"\n
 */

/* Type of XBoard feature*/
enum feature_type { INT_FEAT = 0, TEXT_FEAT };

/* Struct describing an XBoard feature */
struct feature {
  char name[NAME_LENGTH]; /* Name of the feature */
  enum feature_type type; /* Type of value */
  int int_val;            /* Integer value */
  const char *text_val;   /* Text value */
};

/* Set of features this app currently supports */
const struct feature features[] = {
    {"done", INT_FEAT, 0, ""}, /* Don't timeout waiting for further features */
    {"myname", TEXT_FEAT, 0, app_name}, /* The name of the engine */
    {"setboard", INT_FEAT, 0, ""}, /* setboard command is not implemented */
    {"name", INT_FEAT, 0, ""}, /* We don't care about opponent engine's name*/
    {"ping", INT_FEAT, 0, ""}, /* `ping` is not implemented */
    {"variants", TEXT_FEAT, 0,
     "normal"}, /* The only available variant is normal FIDE chess. */
    {"done", INT_FEAT, 1, ""}, /* End of features */
};

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

/* Called when XBoard has notified that a named feature has been accepted */
void feature_accepted(const char *name) {
  for (int i = 0; i < N_FEATURES; i++) {
    if (strcmp(name, features[i].name) == 0) {
      feature_acc[i] = 1;
      break;
    }
  }
}
