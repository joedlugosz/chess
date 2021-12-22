/*
 *    Windows-only functions defined in os.h
 */

#include <conio.h>
#include <windows.h>

#include "os.h"

void init_os(void) {}

/* Setup signal handlers - SIGSEGV is handled and SIGINT is ignored */
void setup_signal_handlers(void) {}

/*
 *    Terminal
 */

/* Check whether an input FILE is a terminal or a file */
int is_terminal(FILE *f) {
  /* Fancy terminal stuff is not supported under Windows */
  return 0;
}

#define BG_MASK (BACKGROUND_BLUE | BACKGROUND_GREEN | BACKGROUND_RED | BACKGROUND_INTENSITY)
#define FG_MASK (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_INTENSITY)

void set_console_attr(DWORD attr, DWORD mask) {
  CONSOLE_SCREEN_BUFFER_INFO cbsi;
  HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
  GetConsoleScreenBufferInfo(h, &cbsi);
  cbsi.wAttributes &= ~mask;
  cbsi.wAttributes |= attr;
  SetConsoleTextAttribute(h, cbsi.wAttributes);
}

void set_console_hilight(void) { set_console_attr(HLITE_SQUARE, BG_MASK); }

void set_console_white_square(void) { set_console_attr(WHITE_SQUARE, BG_MASK); }

void set_console_black_square(void) { set_console_attr(BLACK_SQUARE, BG_MASK); }

void set_console_white_piece(void) { set_console_attr(WHITE_PIECE, FG_MASK); }

void set_console_black_piece(void) { set_console_attr(BLACK_PIECE, FG_MASK); }

/*
 *    Debugging
 */

/* Get OS process ID of this process */
unsigned int get_process_id(void) { return (unsigned int)GetCurrentProcessId(); }

/* Print backtrace to stdout in XBoard format and to a file */
void print_backtrace(FILE *out) {
  /*
    unsigned int   i;
    void         * stack[ 100 ];
    unsigned short frames;
    SYMBOL_INFO  * symbol;
    HANDLE         process;

    process = GetCurrentProcess();
    SymInitialize( process, NULL, TRUE );

    frames               = CaptureStackBackTrace( 0, 100, stack, NULL );
    symbol               = ( SYMBOL_INFO * )calloc( sizeof( SYMBOL_INFO ) + 256 * sizeof( char ), 1
    ); symbol->MaxNameLen   = 255; symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

    for(i = 0; i < frames; i++) {
    SymFromAddr(process, (DWORD64)(stack[i]), 0, &symbol);
    fprintf(out, "%s\n", symbol.Name);
    }
  */
}
