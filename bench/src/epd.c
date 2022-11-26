/* 
 * Runner for EPD test cases
 * Builds executable bench_epd
 * 
 * Runs a set of EPD test cases from a specified file, and prints the results.
 */

#include "cmdline.h"
#include "debug.h"
#include "epd.h"
#include "hash.h"
#include "state.h"

void display_usage(void);

/* 
 * Variables for program arguments 
 */
int depth = 4;
char filename[1000] = "";

/* 
 * Callbacks for program arguments
 */
int arg_depth(struct cmdline *cmdl) {
  const char *arg = cmdline_get(cmdl);
  if (sscanf(arg, "%d", &depth) != 1) return 1;
  return 0;
}

int arg_filename(struct cmdline *cmdl) {
  strcpy(filename, cmdline_get(cmdl));
  return 0;
}

int arg_help(struct cmdline *cmdl) {
  display_usage();
  return 1;
}

/* Table of program arguments */
const struct cmdline_def arg_defs[] = {
  { 0, "", arg_filename, "Input EPD filename", "FILE" },
  { 'd', "depth", arg_depth, "Search depth", "N" },
  { 'h', "help", arg_help, "Display usage info", "" },
  { '?', "", arg_help, "Display usage info" },
  { 0, "", 0, "" },
};

void display_usage(void) {
  printf("Usage:\n\n     bench_epd FILE [OPTIONS]\n\n");
  cmdline_show(arg_defs);
  printf("\n");
}

int main(int argc, const char *argv[]) {
  debug_init();
  init_board();
  hash_init();
  tt_init();
  setbuf(stdout, 0);

  if (cmdline_parse(arg_defs, argc, argv)) return 1;

  if (!filename[0]) {
    printf("\nSpecify an input file\n");
    display_usage();
    return 1;
  }

  return epd_test(filename, depth);
}
