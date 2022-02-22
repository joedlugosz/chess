/*
 *  Debug logging
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifndef NDEBUG
#  define ASSERTS
#endif /* _DEBUG */
#define LOGGING

#ifdef ASSERTS

void assert_fail(const char *file, const char *func, const int line, const char *condition);

#  define ASSERT(x)                                  \
    if (!(x)) {                                      \
      assert_fail(__FILE__, __func__, __LINE__, #x); \
    }
#else
#  define ASSERT(x)
#endif

#ifdef LOGGING
#  include "evaluate.h"
#  define DEBUG_THOUGHT(c, d, s, a, b) debug_thought(c, d, s, a, b)
struct search_job_s_;
void debug_thought(struct search_job_s_ *job, int depth, score_t score, score_t alpha,
                   score_t beta);
#else
#  define DEBUG_THOUGHT(c, d, s, a, b)
#endif /* LOGGING */

#include <stdio.h>
extern FILE *logfile;
void debug_init();
void debug_exit();
void debug_print(const char *fmt, ...);
#endif /* DEBUG_H */
