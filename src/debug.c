/*
 *  Debug logging
 */

#include "debug.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "os.h"

/* Report an error to stderror and as an xboard message to stdout */
void report_error(const char *src_file, const char *func, const int line, const char *msg1,
                  const char *msg2) {
  fprintf(stderr, "Process: %d\n", get_process_id());
  fprintf(stderr, "At: %s %s:%d\n", src_file, func, line);
  fprintf(stderr, "%s %s\n", msg1, msg2);

  printf("{ Process: %d }\n", get_process_id());
  printf("{ At: %s %s:%d }\n", src_file, func, line);
  printf("{ %s %s }\n", msg1, msg2);

  print_backtrace();
}

/* Called when ASSERT() fails. Resign and abort */
void assert_fail(const char *src_file, const char *func, const int line, const char *condition) {
  report_error(src_file, func, line, "Debug assertation failed: ", condition);
  printf("resign\n");
  abort();
}

/* Print alpha, beta, search history */
void debug_thought(struct search_job_s_ *job, int depth, score_t score, score_t alpha, score_t beta) {
  printf("%2d %10d ", depth, job->result.n_leaf);
  if (alpha > -100000)
    printf("%7d ", alpha);
  else
    printf("     -B ");
  if (beta < 100000)
    printf("%7d ", beta);
  else
    printf("     +B ");
  print_thought_moves(depth, job->search_history);
  printf("\n");
}
