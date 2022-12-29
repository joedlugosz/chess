/*
 *  Debug logging and assertion functions
 */

#ifndef DEBUG_H
#define DEBUG_H

#ifndef NDEBUG
#  define ASSERTS
#  define LOGGING
#endif /* _DEBUG */

void debug_init();

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
#  define DEBUG_THOUGHT(j, p, m, d, s, a, b, h) \
    debug_thought(j, p, m, d, s, a, b, h)

struct search_job;
struct pv;
struct move;

void debug_thought(const struct search_job *job, const struct pv *pv,
                   struct move *move, int depth, int score, int alpha, int beta,
                   unsigned long long hash);

void debug_print(const char *fmt, ...);
#else
#  define DEBUG_THOUGHT(j, p, m, d, s, a, b, h)
#endif
#endif /* DEBUG_H */
