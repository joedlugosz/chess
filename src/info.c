/*
 *  Program information
 */

#include "version.h"
#include <stdio.h>

#define str(a) #a
#define stringify(a) str(a)

/* Defined in version.h */
const char version[] = GIT_VERSION;
const char date[] = __DATE__;
const char target[] = TARGET_NAME;
const char os[] = OS_NAME;

/* Defined at invocation */
const char config[] = stringify(CONFIG);

/* Compiler name and version */
#if defined(__clang__)
const char compiler[] = "clang " __VERSION__;
#elif defined(__GNUC__)
const char compiler[] = "gcc " __VERSION__;
#elif defined(_MSC_VER)
const char compiler[] = "MSVC " _MSC_VER;
#else
const char compiler[] = "Unknown";
#endif

/* Program info data */
enum {
  COL_WIDTH = 50
};

typedef struct info_line_s_ {
  char col1[COL_WIDTH];
  const char *col2;
} info_line_s;

const info_line_s lines[] = {
  { "",         0                           },
  { "Joe's Chess Engine", 0                 },
  { "",         0                           },
  { "Version",  version                     },
  { "Config",   config                      },
  { "Built",    date                        },
  { "",         0                           },
  { "Compiler", compiler                    },
  { "Target",   target                      },
  { "OS",       os                          },
  { "",         0                           },
  { "Type 'help' for a list of commands", 0 },
  { "",         0                           }
};

/* Prints a program info banner */
void print_program_info()
{
  char buf[COL_WIDTH+1];
  printf("\n");
  for(int i = 0; i < sizeof(lines)/sizeof(*lines); i++) {
    if(lines[i].col1[0]) {
      if(lines[i].col2) {
        sprintf(buf, "%s:", lines[i].col1);
        printf("%-15s", buf);
        printf("%-60s", lines[i].col2);
      } else {
      	printf("%-75s", lines[i].col1);
      }
    } else {
      printf("%75s", "");
    }
    printf("\n");
  }
  printf("\n");
}
