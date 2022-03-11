
#include "search.h"

#include <stdlib.h>
#include <stdio.h>

#include "hash.c"
#include "history.h"
#include "fen.h"
#include "state.h"

int max_depth = 8;
int ply = 50;

void bench_search(int depth) {
  state_s state;
  struct history history;
  memset(&history, 0, sizeof(history));
  reset_board(&state);

  clock_t total = 0;
  long long n_searched = 0;
  for (int i = 0; i < ply; i++) {
    search_result_s res;

    search(depth, &history, &state, &res, 1);
    total += res.time;
    n_searched += res.n_leaf;

    char fen[100];
    get_fen(&state, fen, sizeof(fen));
    char move[10];
    format_move(move, &res.move, 0);
    printf("move %2d %4d %-70s %8s\n", depth, i, fen, move);
    printf("stat %2d %4d %-70s %8s %10d %4.2lf %16lld %6.2lf\n", depth, i, fen, move, res.n_leaf,
           (double)res.time / 1000000.0, n_searched, (double)total / 1000000.0);

    if (res.move.from == A1 && res.move.to == A1) break;

    make_move(&state, &res.move);
    history_push(&history, state.hash, &res.move);
    change_player(&state);
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
  tt_init();

  for (int i = 1; i <= max_depth; i++) {
    bench_search(i);
  }

  return 0;
}
