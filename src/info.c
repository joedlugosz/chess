/*
 *  Program information
 */

#include <stdio.h>

#include "version.h"

/* Defined in version.h */
const char app_version[] = GIT_VERSION;
const char date[] = __DATE__;
const char target[] = TARGET_NAME;
const char os[] = OS_NAME;

/* Defined at invocation - as quoted strings */
const char config[] = BUILD_CONFIG;
const char options[] = BUILD_FLAGS;

/* Compiler name and version */
#if defined(__clang__)
const char compiler[] = __VERSION__;
#elif defined(__GNUC__)
const char compiler[] = "gcc " __VERSION__;
#elif defined(_MSC_VER)
const char compiler[] = "MSVC " _MSC_VER;
#else
const char compiler[] = "Unknown";
#endif

/* Program info data */
enum { COL_WIDTH = 50 };

typedef struct info_line_s_ {
  char key[COL_WIDTH];
  const char *value;
} info_line_s;

/* clang-format off */
const info_line_s lines[] = {
  { "",         0                           },
  { "Joe's Chess Engine", 0                 },
  { "",         0                           },
  { "Version",  app_version                 },
  { "Config",   config                      },
  { "Built",    date                        },
  { "",         0                           },
  { "Compiler", compiler                    },
  { "Options",  options                     },
  { "Target",   target                      },
  { "OS",       os                          },
  { "",         0                           },
  { "Type 'help' for a list of commands", 0 },
  { "",         0                           }
};
/* clang-format on */

/* Prints a program info banner */
void print_program_info() {
  char buf[COL_WIDTH + 1];
  printf("\n");
  for (int i = 0; i < sizeof(lines) / sizeof(*lines); i++) {
    if (lines[i].key[0]) {
      if (lines[i].value) {
        sprintf(buf, "%s:", lines[i].key);
        printf("%-15s", buf);
        printf("%-60s", lines[i].value);
      } else {
        printf("%-75s", lines[i].key);
      }
    } else {
      printf("%75s", "");
    }
    printf("\n");
  }
  printf("\n");
}
