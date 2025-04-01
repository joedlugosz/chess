/*
 * Search benchmarking app
 * Builds executable bench_search
 *
 * Plays AI-AI games at increasing depths and prints moves and statistics.
 */

#include "search.h"

#include <stdio.h>
#include <stdlib.h>

#include "fen.h"
#include "hash.c"
#include "history.h"
#include "io.h"
#include "position.h"

int max_depth = 8;
int ply = 50;

void bench_search(int depth) {
  struct position position;
  reset_board(&position);

  struct history history;
  memset(&history, 0, sizeof(history));

  clock_t total = 0;
  long long n_searched = 0;

  for (int i = 0; i < ply; i++) {
    struct search_result res;

    search(depth, 0.0, 0.0, &history, &position, &res, 1);
    total += res.time;
    n_searched += res.n_leaf;

    char fen[100];
    get_fen(&position, fen, sizeof(fen));
    char move[10];
    format_move(move, &res.move, 0);

    printf("move %2d %4d %-70s %8s\n", depth, i, fen, move);
    printf("stat %2d %4d %-70s %8s %10d %4.2lf %16lld %6.2lf\n", depth, i, fen,
           move, res.n_leaf, (double)res.time / 1000000.0, n_searched,
           (double)total / 1000000.0);

    if (res.move.from == A1 && res.move.to == A1) break;

    struct unmake unmake;
    make_move(&position, &res.move, &unmake);
    history_push(&history, position.hash, &res.move);
    change_player(&position);
  }

  printf("avg  %4d %4d %16lld %6.2lf\n", depth, ply, n_searched / ply,
         (double)total / ((double)ply * 1000000.0));
}

int main(int argc, const char *argv[]) {
  if (argc == 2) {
    printf("%s\n", argv[1]);
    if (sscanf(argv[1], "%d", &max_depth) != 1) {
      return 1;
    }
  }

  setbuf(stdout, 0);
  init_board();
  hash_init();
  debug_init();
  tt_init();

  for (int i = 1; i <= max_depth; i++) {
    bench_search(i);
  }

  return 0;
}
