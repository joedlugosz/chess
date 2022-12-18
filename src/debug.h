/*
 *  Debug logging and assertion functions
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifndef NDEBUG
#  define ASSERTS
//#  define LOGGING
#endif /* _DEBUG */

#ifdef ASSERTS

void assert_fail(const char *file, const char *func, const int line,
                 const char *condition);

#  define ASSERT(x)                                  \
    if (!(x)) {                                      \
      assert_fail(__FILE__, __func__, __LINE__, #x); \
    }
#else
#  define ASSERT(x)
#endif

#ifdef LOGGING
#  include "evaluate.h"
#  define DEBUG_THOUGHT(j, p, d, s, a, b, h) debug_thought(j, p, d, s, a, b, h)

struct search_job;
struct pv;

void debug_thought(const struct search_job *job, const struct pv *pv, int depth,
                   score_t score, score_t alpha, score_t beta,
                   unsigned long long hash);

#else
#  define DEBUG_THOUGHT(j, p, d, s, a, b, h)
#endif /* LOGGING */

#include <stdio.h>
extern FILE *logfile;
void debug_init();
void debug_exit();
void debug_print(const char *fmt, ...);
#endif /* DEBUG_H */
