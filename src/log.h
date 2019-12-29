#ifndef LOG_H
#define LOG_H

#include "options.h"
#include <stdio.h>

typedef enum newevery_e_ {
  NE_NEVER = 0, NE_SESSION, NE_GAME, NE_MOVE, NE_N
} newevery_e;
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

# if (LOGGING == YES) 
void start_log(log_s *log, newevery_e new_log, const char *fmt, ...);
void print_log(log_s *log, const char *fmt, ...);
void assert_fail(log_s *log, const char *file, const char *func, const int line, const char *condition);
#  define ASSERT(x) if(!(x)) { assert_fail(&error_log, __FILE__, __func__, __LINE__, #x); }
#  define START_LOG(l, n, f, ...) start_log(l, n, f, __VA_ARGS__)
#  define PRINT_LOG(l, f, ...) print_log(l, f, __VA_ARGS__)
# else
#  define ASSERT(x)
#  define START_LOG(l, n, f, ...)
#  define PRINT_LOG(l, f, ...)
# endif /* LOGGING == YES */
void set_sigsegv_handler(void);
int open_log(log_s *log);
void close_log(log_s *log);
#endif /* LOG_H */
