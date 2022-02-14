// /*
//  *  Debug logging
//  */

#include "debug.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "io.h"
#include "options.h"
#include "os.h"
#include "search.h"

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

void assert_fail(const char *src_file, const char *func, const int line, const char *condition) {
  report_error(src_file, func, line, "Debug assertation failed: ", condition);
  printf("resign\n");
  abort();
}

/* Thought logging */

void debug_thought(search_job_s *job, int depth, score_t score, score_t alpha, score_t beta) {
  printf("\n%2d %10d ", depth, job->result.n_leaf);
  if (alpha > -100000)
    printf("%7d ", alpha);
  else
    printf("     -B ");
  if (beta < 100000)
    printf("%7d ", beta);
  else
    printf("     +B ");
  print_thought_moves(depth, job->search_history);
}
