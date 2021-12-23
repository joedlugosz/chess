/*
 *  Debug logging
 */

#include "log.h"

#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "options.h"
#include "os.h"

#ifndef LOG_DIR
#  define LOG_DIR ""
#endif
#ifndef PROGRAM_NAME
#  define PROGRAM_NAME "chess"
#endif

/*
 *  Options
 */

const combo_val_s newevery_vals[NE_N] = {{"Never", NE_NEVER},
                                         {"Every session", NE_SESSION},
                                         {"Every game", NE_GAME},
                                         {"Every move", NE_MOVE}};
const combo_s newevery_combo = {NE_N, newevery_vals};

const options_s log_opts = {0, 0};

void start_log(log_s *log, newevery_e new_log, const char *fmt, ...) {
  va_list args;
  char titlebuf[100];

  if (new_log == log->new_every) {
    va_start(args, fmt);
    vsnprintf(titlebuf, sizeof(titlebuf), fmt, args);
    snprintf(log->path, sizeof(log->path), "%ss%05d-%s.log", LOG_DIR, get_process_id(), titlebuf);
    errno = 0;
    log->file = fopen(log->path, "w");
    if (log->file == NULL) {
      fprintf(stderr, "%s: Could not open file %s: %s\n", PROGRAM_NAME, log->path, strerror(errno));
      exit(errno);
    } else {
      log->logging = 1;
      fclose(log->file);
    }
  }
}

int open_log(log_s *log) {
  if (log->logging) {
    log->file = fopen(log->path, "a");
    if (log->file) return 0;
  }
  return 1;
}

void close_log(log_s *log) {
  if (log->file && log->logging) {
    fclose(log->file);
  }
}

void print_log(log_s *log, const char *fmt, ...) {
  va_list args;

  if (log->logging) {
    if (!open_log(log)) {
      fprintf(log->file, "\n");
      va_start(args, fmt);
      vfprintf(log->file, fmt, args);
      close_log(log);
    }
  }
}

void log_error(log_s *log, const char *file, const char *func, const int line, const char *msg1,
               const char *msg2) {
  FILE *out;

  if (!open_log(log)) {
    out = log->file;
  } else {
    out = stdout;
  }
  fprintf(out, "Process: %d\n", get_process_id());
  fprintf(out, "At: %s %s:%d\n", file, func, line);
  fprintf(out, "%s %s\n", msg1, msg2);
  printf("{Process: %d}\n", get_process_id());
  printf("{At: %s %s:%d}\n", file, func, line);
  printf("{%s %s}\n", msg1, msg2);
  print_backtrace(out);
  if (out != stdout) close_log(log);
}

void assert_fail(log_s *log, const char *file, const char *func, const int line,
                 const char *condition) {
  log_error(log, file, func, line, "Debug assertation failed: ", condition);
  printf("resign\n");
  abort();
}
