/*
 *    POSIX-only functions defined in os.h
 */

/* For siginfo_t */
#define _POSIX_C_SOURCE 200809L

#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "log.h"
#include "os.h"

void init_os(void) {}

/* SIGSEGV handler */
static void sigsegv_handler(int sig, siginfo_t *si, void *unused) {
  PRINT_LOG(&xboard_log, "%s", "Received sigsegv");
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

/* Print backtrace to stdout in XBoard format and to a file */
void print_backtrace(FILE *out) {
  /* Get backtrace symbols */
  void *bt[1024];
  int n_bt = backtrace(bt, 1024);
  char **bt_syms = backtrace_symbols(bt, n_bt);

  /* Look up function names and line numbers */
  char bt_function[1000][1024];
  char bt_line[1000][1024];

  for (int i = 1; i < n_bt; i++) {
    /* Symbol in name(offset) format is at the start of each backtrace line */
    char symbol[256];
    sscanf(bt_syms[i], "%s", symbol);

    /* Convert address to format for addr2line
     *     name(+0x1234).
     *  -> name +0x1234..  */
    symbol[strlen(symbol) - 9] = ' ';
    symbol[strlen(symbol) - 1] = 0;

    /* Run addr2line to get function names and line numbers */
    char cmd[1000];
    snprintf(cmd, sizeof(cmd), "addr2line -f -e %s", ((const char *)symbol));
    FILE *out = popen(cmd, "r");
    if (out) {
      if (!fgets(bt_function[i], sizeof(bt_function[i]), out)) sprintf(bt_function[i], "%s", "???");
      bt_function[i][strlen(bt_function[i]) - 1] = 0;
      if (!fgets(bt_line[i], sizeof(bt_line[i]), out)) sprintf(bt_line[i], "%s", "???");
      bt_line[i][strlen(bt_line[i]) - 1] = 0;
      pclose(out);
    }
  }

  /* Print backtrace */
  if (out) fprintf(out, "\nCall stack:\n");
  printf("\n{Call stack:}\n");
  for (int i = 2; i < n_bt; i++) {
    if (out) fprintf(out, "%s %-20s %s\n", bt_syms[i], bt_function[i], bt_line[i]);
    printf("{%s %20s %s}\n", bt_syms[i], bt_function[i], bt_line[i]);
  }
}
