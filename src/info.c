/*
 *  Program information
 */

#include <stdio.h>

#include "buildinfo/buildinfo.h"

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
  { "Version",  git_version                 },
  { "Config",   build_config                },
  { "Built",    build_date                  },
  { "",         0                           },
  { "Compiler", compiler_name               },
  { "Options",  compiler_options            },
  { "Target",   target_name                 },
  { "OS",       os_name                     },
  { "",         0                           },
  { "Type 'help' for a list of commands", 0 },
  { "",         0                           }
};
/* clang-format on */

/* Prints a program info banner */
void print_program_info() {
  char buf[1000];
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
