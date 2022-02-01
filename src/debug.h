/*
 *  Debug logging
 */

#ifndef LOG_H
#define LOG_H

#include <stdio.h>

#include "options.h"

typedef enum newevery_e_ { NE_NEVER = 0, NE_SESSION, NE_GAME, NE_MOVE, NE_N } newevery_e;
extern const combo_s newevery_combo;

typedef struct log_s_ {
  FILE *file;
  int logging;
  char path[1000];
  newevery_e new_every;
} log_s;
extern log_s xboard_log;
extern log_s think_log;
extern log_s error_log;

#ifdef ASSERTS

void assert_fail(log_s *log, const char *file, const char *func, const int line,
                 const char *condition);

#  ifdef LOGGING
#    define ASSERT(x)                                              \
      if (!(x)) {                                                  \
        assert_fail(&error_log, __FILE__, __func__, __LINE__, #x); \
      }

#  else
#    define ASSERT(x)                                     \
      if (!(x)) {                                         \
        assert_fail(0, __FILE__, __func__, __LINE__, #x); \
      }

#  endif
#else
#  define ASSERT(x)
#endif

#ifdef LOGGING
void start_log(log_s *log, newevery_e new_log, const char *fmt, ...);
void print_log(log_s *log, const char *fmt, ...);
#  define LOG_THOUGHT(c, d, s, a, b) log_thought(&think_log, c, d, s, a, b)
#  define START_LOG(l, n, f, ...) start_log(l, n, f, __VA_ARGS__)
#  define PRINT_LOG(l, f, ...) print_log(l, f, __VA_ARGS__)

void debug_thought(FILE *f, search_job_s *job, int depth, score_t score, score_t alpha,
                   score_t beta);
#else
#  define LOG_THOUGHT(c, d, s, a, b)
#  define START_LOG(l, n, f, ...)
#  define PRINT_LOG(l, f, ...)
#endif /* LOGGING */

void set_sigsegv_handler(void);
int open_log(log_s *log);
void close_log(log_s *log);

#endif /* LOG_H */
