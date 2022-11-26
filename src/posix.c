/*
 *    POSIX-only functions defined in os.h
 */

/* For siginfo_t */
#define _POSIX_C_SOURCE 200809L

#include <execinfo.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "os.h"

void init_os(void) {}

/* SIGSEGV handler */
static void sigsegv_handler(int sig, siginfo_t *si, void *unused) {
  printf("%s\n", "{ Received sigsegv }");
  fprintf(stderr, "%s\n", "{ Received sigsegv }");
  print_backtrace(0);
  abort();
}

/* Setup signal handlers - SIGSEGV is handled */
void setup_signal_handlers(void) {
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = sigsegv_handler;

  if (sigaction(SIGSEGV, &sa, 0) == -1) {
    printf("Can't set sigsegv handler");
  }
}

/* SIGINT is ignored for XBoard mode */
void ignore_sigint(void) { signal(SIGINT, SIG_IGN); }

/*
 *    Terminal
 */

/* Check whether an input FILE is a terminal or a file */
int is_terminal(FILE *f) { return isatty(fileno(f)); }

void set_console_hilight1(void) { printf("%s", HLITE1_SQUARE); }

void set_console_hilight2(void) { printf("%s", HLITE2_SQUARE); }

void set_console_white_square(void) { printf("%s", WHITE_SQUARE); }

void set_console_black_square(void) { printf("%s", BLACK_SQUARE); }

void set_console_white_piece(void) { printf("%s", WHITE_PIECE); }

void set_console_black_piece(void) { printf("%s", BLACK_PIECE); }

/*
 *    Debugging
 */

/* Get OS process ID of this process */
unsigned int get_process_id(void) { return (unsigned int)getpid(); }

typedef struct bt_entry_s_ {
  char module[1000];
  char function[1000];
  uintptr_t rel_addr;
  uintptr_t abs_addr;
  char a2l_function[1000];
  char line[1000];
} bt_entry_s;

/* Print backtrace to stdout in XBoard format and to a file */
void print_backtrace(FILE *f) {
  /* Get backtrace symbols */
  void *bt_raw[1024];
  int n_bt = backtrace(bt_raw, 1024);
  char **bt_syms = backtrace_symbols(bt_raw, n_bt);

  /* Parse backtrace symbols text
   *
   *                     rel_addr      abs_addr
   *     module(function+0x1234) [0x12345678]
   * or  module(+0x1234) [0x12345678]
   */

  bt_entry_s bt[1000];

  for (int i = 0; i < n_bt; i++) {
    char *start = bt_syms[i];
    char *ptr = start;

    /* module */
    while (*ptr != '(') {
      ptr++;
    }
    *ptr++ = 0;
    strcpy(bt[i].module, start);
    start = ptr;

    /* function (if given) */
    if (*ptr != '+') {
      while (*ptr != '+') {
        ptr++;
      }
      *ptr++ = 0;
      strcpy(bt[i].function, start);
    } else {
      bt[i].function[0] = 0;
      *ptr++ = 0;
    }
    start = ptr;

    /* Look up function names and line numbers */
    /* rel_addr */
    while (*ptr != ')') {
      ptr++;
    }
    *ptr++ = 0;
    sscanf(start, "%zx", &bt[i].rel_addr);
    start = ptr;

    /* text ') [' - ignore */
    while (*ptr != '[') {
      ptr++;
    }
    *ptr++ = 0;
    start = ptr;

    /* abs_addr */
    while (*ptr != ']') {
      ptr++;
    }
    *ptr++ = 0;
    sscanf(start, "%zx", &bt[i].abs_addr);

    char cmd[4000];

    /* Run nm to look up base address of function, and add to rel_addr */
    uintptr_t base_addr = 0;
    if (bt[i].function[0]) {
      snprintf(cmd, sizeof(cmd), "nm %s 2>/dev/null | grep -w %s",
               ((const char *)bt[i].module), bt[i].function);
      FILE *proc = popen(cmd, "r");
      if (proc) {
        if (!fscanf(proc, "%zx", &base_addr)) {
          if (pclose(proc) == 0) {
            bt[i].rel_addr += base_addr;
          }
        }
      }
    }

    /* Run addr2line to get function names and line numbers */
    snprintf(cmd, sizeof(cmd), "addr2line -f -s -e %s %p",
             ((const char *)bt[i].module), (void *)bt[i].rel_addr);
    FILE *proc = popen(cmd, "r");
    if (proc) {
      if (!fgets(bt[i].a2l_function, sizeof(bt[i].a2l_function), proc))
        sprintf(bt[i].a2l_function, "%s", "???");
      bt[i].a2l_function[strlen(bt[i].a2l_function) - 1] = 0;
      if (!fgets(bt[i].line, sizeof(bt[i].line), proc))
        sprintf(bt[i].line, "%s", "???");
      bt[i].line[strlen(bt[i].line) - 1] = 0;
      pclose(proc);
    }

    /* If backtrace_symbols didn't return a function name, use the one from
     * addr2line */
    if (!bt[i].function[0]) strcpy(bt[i].function, bt[i].a2l_function);
  }

  /* Print backtrace */
  fprintf(logfile, "\n{ Call stack: }\n");
  for (int i = 3; i < n_bt; i++) {
    fprintf(logfile, "{  %-40s %-20s %s }\n", bt_syms[i], bt[i].function,
            bt[i].line);
    if (strcmp(bt[i].function, "main") == 0) break;
  }
}
