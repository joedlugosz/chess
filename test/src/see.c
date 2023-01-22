#include "see.h"

#include "fen.h"
#include "hash.h"
#include "test.h"

int test_see() {
  struct position position;

  load_fen(&position, "rnbqkbnr/pppp1p1p/4p3/6p1/8/3P4/PPP1PPPP/RNBQKBNR", "w",
           "KQkq", "-", "0", "1");
  TEST_ASSERT(see_after_move(&position, C1, G5, BISHOP) == -2,
              "SEE result is PAWN-BISHOP for white bishop taking black pawn "
              "defended by queen");

  load_fen(&position, "rnbqkbnr/pppppp1p/8/6p1/8/3P4/PPP1PPPP/RNBQKBNR", "w",
           "KQkq", "-", "0", "1");
  TEST_ASSERT(see_after_move(&position, C1, G5, BISHOP) == 1,
              "SEE result is PAWN for white bishop taking undefended black "
              "pawn");

  load_fen(&position, "rnbqkbnr/pppp1p1p/4p3/6p1/8/3P1N2/PPP1PPPP/RNBQKB1R",
           "w", "KQkq", "-", "0", "2");
  TEST_ASSERT(see_after_move(&position, C1, G5, BISHOP) == 1,
              "SEE result is PAWN for white bishop taking black pawn "
              "defended by queen where there is also a white knight to take "
              "the queen.");

  return 0;
}

int main(void) {
  init_board();
  hash_init();
  tt_init();
  test_init(1, "see");
  test_see();
  return 0;
}
