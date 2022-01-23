
#include <stdlib.h>

#include "fen.h"
#include "search.h"
#include "state.h"

void bench_search(void) {
  
  state_s state;
  reset_board(&state);

  clock_t total = 0;
  long long n_searched = 0;
  for (int i = 0; i < 50; i++) {

    search_result_s res;

    search(6, &state, &res);
    total += res.time;
    n_searched += res.n_searched;

    char fen[100];
    get_fen(&state, fen, sizeof(fen));
    char move[10];
    format_move(move, &res.move, 0);
    printf("move %4d %-70s %8s\n", i, fen, move);
    printf("stat %4d %-70s %8s %10d %4.2lf %16lld %6.2lf\n", i, fen, move, res.n_searched, 
      (double)res.time / 1000000.0, n_searched, (double)total / 1000000.0);

    if (res.move.from == A1 && res.move.to == A1) break;

    make_move(&state, &res.move);
    change_player(&state);
    
  }

}

int main(void) {

  setbuf(stdout, 0);
  init_board();
  bench_search();

  return 0;
}
