/*
 *  Program build info banner
 */

#include <stdio.h>

#include "buildinfo/buildinfo.h"

enum { COL_WIDTH = 50 };

/* A line of program information */
struct info_line {
  char key[COL_WIDTH];
  const char *value;
};

/* clang-format off */

/* The lines to display */
const struct info_line lines[] = {
  { "",              0                           },
  { "Joe's Chess Engine", 0                      },
  { "",              0                           },
  { "Version",       git_version                 },
  { "Source date",   source_date                 },
  { "",              0                           },
  { "Configuration", build_config                },
  { "Compiler",      compiler_name               },
  { "Options",       compiler_options            },
  { "Target",        target_name                 },
  { "OS",            os_name                     },
  { "Build date",    build_date                  },
  { "",              0                           },
  { "Type 'help' for a list of commands", 0      },
  { "",              0                           }
};
/* clang-format on */

/* Print a program info banner */
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
