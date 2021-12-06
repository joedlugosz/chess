/*
 * 4.1  18/02/18  New 
 */

#include "compiler.h"
#include "os.h"
#include <stdio.h>

const char date[] = __DATE__;
const char version[] = "4.1";
char compiler[100];
const char target[] = TARGET_NAME;
const char os_text[2][10] = { "Linux", "Windows" };
const char cpu[] = "";

enum {
  N_BANNER_LINES = 13,
  COL_WIDTH = 50
};
typedef struct banner_line_s_ {
  char col1[COL_WIDTH];
  const char *col2;
} banner_line_s;

const banner_line_s lines[N_BANNER_LINES] = {
  { "",         0                           },
  { "Joe's Chess Engine", 0                 },
  { "",         0                           },
  { "Version",  "4.1"                       },
  { "Built",    date                        },
  { "",         0                           },
  { "Compiler", compiler                    },
  { "Target",   target                      },
  { "CPU",      cpu                         },
  { "OS",       os_text[OS]                 },
  { "",         0                           },
  { "Type 'help' for a list of commands", 0 },
  { "",         0                           }
};

void print_program_info()
{
  char buf[COL_WIDTH+1];
#if (COMP==MSVC)
  sprintf(compiler, "Microsoft Visual C++ %lu", _MSC_FULL_VER);
#else
  sprintf(compiler, "%s", COMPILER_NAME);
# endif
  printf("\n");
  for(int i = 0; i < N_BANNER_LINES; i++) {
    set_console_black_square();
    printf("  ");
    set_console_hilight1();
    printf("  ");
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
    set_console_black_square();
    printf("\n");
  }
  set_console_black_square();
  printf("\n");
}
