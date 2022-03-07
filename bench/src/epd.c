#include "cmdline.h"
#include "debug.h"
#include "epd.h"
#include "hash.h"
#include "state.h"

int depth = 4;
char filename[1000] = "";

int arg_depth(struct cmdline *cmdl) {
  const char *arg = cmdline_get(cmdl);
  if (sscanf(arg, "%d", &depth) != 1) return 1;
  return 0;
}

int arg_filename(struct cmdline *cmdl) {
  strcpy(filename, cmdline_get(cmdl));
  return 0;
}

void usage(void);

int arg_help(struct cmdline *cmdl) {
  usage();
  return 1;
}

const struct cmdline_def arg_defs[] = {
  { 0, "", arg_filename, "Input EPD filename", "FILE" },
  { 'd', "depth", arg_depth, "Search depth", "N" },
  { 'h', "help", arg_help, "Display usage info", "" },
  { '?', "", arg_help, "Display usage info" },
  { 0, "", 0, "" },
};

void usage(void) {
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
    usage();
    return 1;
  }

  return epd_test(filename, depth);
}
