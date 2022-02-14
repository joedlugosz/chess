/*
 *  Debug logging
 */

#ifndef LOG_H
#define LOG_H

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
#  define DEBUG_THOUGHT(c, d, s, a, b) debug_thought(c, d, s, a, b)

void debug_thought(search_job_s *job, int depth, score_t score, score_t alpha, score_t beta);
#else
#  define DEBUG_THOUGHT(c, d, s, a, b)
#endif /* LOGGING */

void set_sigsegv_handler(void);

#endif /* LOG_H */
