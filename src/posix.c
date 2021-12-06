#include "compiler.h"
#include "os.h"
#include "log.h"
//#include "chess.h"

#include <sys/select.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

void init_os(void) {
  signal(SIGINT, SIG_IGN);
}

file_t get_stdin(void) {
  return FD_STDIN;
}

unsigned int get_process_id(void) {
  return (unsigned int)getpid();
}

/* Non-blocking read from connection */
ssize_t conn_read_nb(file_t file, char *buf, ssize_t buf_size)
{
  ssize_t n_read;
  struct timeval tv;
  fd_set fds;
  tv.tv_sec = 0;
  tv.tv_usec = 1000;
  
  n_read = 0;
  buf[0] = 0;
  FD_SET(file, &fds);
  if(select(file + 1, &fds, 0, 0, &tv) > 0) {
    if(FD_ISSET(file, &fds)) {
      n_read = read(file, buf, buf_size);
      if(n_read == -1) n_read = 0;
    }
  }
  
  return n_read;
}

/* Check whether an input FILE is a terminal or a file */
int is_terminal(FILE *f) {
#if (TERMINAL)
  return isatty(fileno(f));
#else
  return 0;
#endif
}

void print_backtrace(FILE *out)
{
  void *bt[1024];
  char **bt_syms;
  int n_bt, i;
  n_bt = backtrace(bt, 1024);
  bt_syms = backtrace_symbols(bt, n_bt);
  fprintf(out, "\nCall stack:\n");
  printf("\n{Call stack:}\n");
  for(i = 2; i < n_bt; i++) {
    fprintf(out, "%s\n", bt_syms[i]);
    printf("{%s}\n", bt_syms[i]);
  }
  /*
  for(i = 1; i < n_bt; i++) {
    char exename[256];
    sscanf(bt_syms[i], "%s", exename);
    char cmd[256];
    snprintf(cmd, 256, "addr2line -f -e %s %p", ((const char const *)exename), bt[i]);
    printf("%s\n", cmd);
    if(system(cmd)) {
    }
    }*/
}

static void sigsegv_handler(int sig, siginfo_t *si, void *unused)
{
  PRINT_LOG(&xboard_log, "%s", "Received sigsegv");
  abort();
}

static void sigint_handler(int sig, siginfo_t *si, void *unused)
{
  //char buf[100];
  //buf[0]=0;
  //(void)scanf("%s", buf);
  //log_error(&xboard_log, "Interrupt: ", buf);
}

void set_sigsegv_handler(void)
{
  struct sigaction sa;
  sa.sa_flags = SA_SIGINFO;
  sigemptyset(&sa.sa_mask);
  sa.sa_sigaction = sigsegv_handler;
  if(sigaction(SIGSEGV, &sa, 0) == -1) {
  }
  sa.sa_sigaction = sigint_handler;
  if(sigaction(SIGINT, &sa, 0) == -1) {
  }
}


void set_console_hilight1(void) 
{
  printf("%s", HLITE1_SQUARE);
}
	
void set_console_hilight2(void) 
{
  printf("%s", HLITE2_SQUARE);
}
	
void set_console_white_square(void) 
{
  printf("%s", WHITE_SQUARE);
}

void set_console_black_square(void)
{
  printf("%s", BLACK_SQUARE);
}

void set_console_white_piece(void)
{
  printf("%s", WHITE_PIECE);
}

void set_console_black_piece(void)
{
  printf("%s", BLACK_PIECE);
}


/*
  void sigint_handler(int dummy)
  {
  signal(SIGINT, SIG_IGN);
  stop_search();
  signal(SIGINT, sigint_handler);
  }
  int check_input()
  {
  fd_set fds;
  FD_SET(0, &fds);
  select(0, &fds, 0, 0, 0);
  if(FD_ISSET(0, &fds)) {
  return 1;
  }
  return 0;
  }
  void sigterm_handler(int dummy)
  {
  signal(SIGTERM, SIG_IGN);
  stop_search();
  run = 0;
  signal(SIGTERM, sigterm_handler);
  }
*/

