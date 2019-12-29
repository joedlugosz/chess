/*
 *   4.1 New - Windows supported
 */
#define POSIX 0
#define WIN 1

#define TERM_COLOURS 1

#if (OS == POSIX)
# define TERM_UNICODE 1

# if(TERM_COLOURS)
#  define WHITE_SQUARE "\033[44m"
#  define BLACK_SQUARE "\033[0m"
#  define HLITE1_SQUARE "\033[42m"
#  define HLITE2_SQUARE "\033[43m"
#  define WHITE_PIECE  "\033[97m"
#  define BLACK_PIECE  "\033[39m"
# else
#  define WHITE_SQUARE ""
#  define BLACK_SQUARE ""
#  define HLITE1_SQUARE ""
#  define HLITE2_SQUARE "\033[42m"
#  define WHITE_PIECE  ""
#  define BLACK_PIECE  ""
# endif


/* For fileno() in stdio */
//# if (TERMINAL)
# define _POSIX_SOURCE
//# endif
# include <unistd.h>
typedef int 	file_t;		/* Int file descriptor */
typedef ssize_t filesize_t;
typedef pid_t   procid_t;
# define FD_STDIN 0

#elif (OS == WIN)
# include <windows.h>
# include <wincon.h>
# define TERM_UNICODE 0

# if(TERM_COLOURS)
#  define WHITE_SQUARE (BACKGROUND_BLUE | FOREGROUND_INTENSITY)
#  define BLACK_SQUARE FOREGROUND_INTENSITY
#  define HLITE_SQUARE (BACKGROUND_GREEN | FOREGROUND_INTENSITY)
#  define BLACK_PIECE FOREGROUND_INTENSITY
#  define WHITE_PIECE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY)
 //#  define WHITE_SQUARE 4
//#  define BLACK_SQUARE 0
//#  define HLITE_SQUARE 5
# else
#  define WHITE_SQUARE BACKGROUND_BLACK
#  define BLACK_SQUARE BACKGROUND_BLACK
#  define HLITE_SQUARE BACKGROUND_BLACK
# endif


typedef HANDLE 	file_t;
typedef DWORD 	filesize_t;
typedef DWORD	procid_t;

#endif /* OS */

/* Configuration logic */
#if ((TERM_UNICODE | TERM_COLOURS))
# define TERMINAL 1
#else
# define TERMINAL 0
#endif

#if(TERMINAL)
//# include <unistd.h>
#endif


#include <stdio.h>

/* These functions are all defined in an OS-specific module.
 *    posix.c
 *    win32.c
 */
void init_os(void);
file_t get_stdin(void);
unsigned int get_process_id(void);
filesize_t conn_read_nb(file_t file, char *buf, filesize_t buf_size);
int is_terminal(FILE *f);
void print_backtrace(FILE *out);
void set_sigsegv_handler(void);
void set_console_hilight1(void);
void set_console_hilight2(void);
void set_console_white_square(void);
void set_console_black_square(void);
void set_console_white_piece(void);
void set_console_black_piece(void);

