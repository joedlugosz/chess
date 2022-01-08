#ifndef OS_H
#define OS_H

#define POSIX 0
#define WIN 1

#define TERM_COLOURS 1

#if defined(_WINDOWS)
#  include <windows.h>
#  define TERM_UNICODE 0

#  if (TERM_COLOURS)
#    define WHITE_SQUARE (BACKGROUND_BLUE | FOREGROUND_INTENSITY)
#    define BLACK_SQUARE FOREGROUND_INTENSITY
#    define HLITE1_SQUARE (BACKGROUND_GREEN | FOREGROUND_INTENSITY)
#    define HLITE2_SQUARE (BACKGROUND_GREEN | BACKGROUND_RED | FOREGROUND_INTENSITY)
#    define BLACK_PIECE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#    define WHITE_PIECE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
#  else
#    define WHITE_SQUARE BACKGROUND_BLACK
#    define BLACK_SQUARE BACKGROUND_BLACK
#    define HLITE_SQUARE BACKGROUND_BLACK
#  endif

#else
#  define TERM_UNICODE 1

#  if (TERM_COLOURS)
#    define WHITE_SQUARE "\033[44m"
#    define BLACK_SQUARE "\033[0m"
#    define HLITE1_SQUARE "\033[42m"
#    define HLITE2_SQUARE "\033[43m"
#    define WHITE_PIECE "\033[97m"
#    define BLACK_PIECE "\033[39m"
#  else
#    define WHITE_SQUARE ""
#    define BLACK_SQUARE ""
#    define HLITE1_SQUARE ""
#    define HLITE2_SQUARE "\033[42m"
#    define WHITE_PIECE ""
#    define BLACK_PIECE ""
#  endif

#endif /* OS */

#include <stdio.h>

/* These functions are all defined in an OS-specific module.
 *    posix.c
 *    win32.c
 */
void setup_signal_handlers(void);
void ignore_sigint(void);
int is_terminal(FILE *f);
void set_console_hilight1(void);
void set_console_hilight2(void);
void set_console_white_square(void);
void set_console_black_square(void);
void set_console_white_piece(void);
void set_console_black_piece(void);
unsigned int get_process_id(void);
void print_backtrace(FILE *out);

#endif /* OS_H */
