/*
 *  Debug logging and assertion functions
 */

#include "debug.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "os.h"
#include "position.h"
#include "search.h"

/* File handle used for logging */
FILE *logfile;

#define USE_LOGFILE 0
#define LOGFILE_PID 0

/* Initialise logging. Call at program init. */
void debug_init() {
  if (USE_LOGFILE) {
    char filename[100];
    if (LOGFILE_PID) {
      int pid = get_process_id();
      sprintf(filename, "%d.log", pid);
    } else {
      sprintf(filename, "chess.log");
    }
    logfile = fopen(filename, "a");
    setbuf(logfile, NULL);
    if (!logfile) {
      perror("debug_init:");
      exit(1);
    }
  } else {
    logfile = stdout;
  }
}

/* Print a debug message to log file or stdout */
void debug_print(const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);
  if (logfile) {
    vfprintf(logfile, fmt, args);
  }
}

/* Close logging. Call at program exit. */
void debug_exit() {
  if (logfile && logfile != stdout) {
    fclose(logfile);
  }
}

/* Report an error to stderror and as an xboard message to stdout */
static void report_error(const char *src_file, const char *func, const int line,
                         const char *msg1, const char *msg2) {
  if (logfile != stdout) {
    fprintf(stderr, "Process: %d\n", get_process_id());
    fprintf(stderr, "At: %s %s:%d\n", src_file, func, line);
    fprintf(stderr, "%s %s\n", msg1, msg2);
  }

  printf("{ Process: %d }\n", get_process_id());
  printf("{ At: %s %s:%d }\n", src_file, func, line);
  printf("{ %s %s }\n", msg1, msg2);

  print_backtrace();
}

/* Called when ASSERT() fails. Resign and abort */
void assert_fail(const char *src_file, const char *func, const int line,
                 const char *condition) {
  report_error(src_file, func, line, "Debug assertation failed: ", condition);
  printf("resign\n");
  abort();
}

/* Print alpha, beta, search history to logfile or stdout */
void debug_thought(const struct search_job *job, const struct pv *pv, int depth,
                   score_t score, score_t alpha, score_t beta, hash_t hash) {
  fprintf(logfile, "%2d %10d %d", depth, job->result.n_leaf, score);
  if (alpha > -100000)
    fprintf(logfile, "%7d ", alpha);
  else
    fprintf(logfile, "     -B ");
  if (beta < 100000)
    fprintf(logfile, "%7d ", beta);
  else
    fprintf(logfile, "     +B ");
  fprintf(logfile, "  %016llx ", hash);
  print_pv(logfile, pv);
  fprintf(logfile, "\n");
}
