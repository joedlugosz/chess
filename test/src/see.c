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

  load_fen(&position, "rnbqkb1r/ppppp1pp/8/5p2/6n1/1P2P3/P1PPBPPP/RNB1K1NR",
           "w", "KQkq", "f6", "0", "4");
  TEST_ASSERT(see_after_move(&position, E2, G4, BISHOP) == 0,
              "*Ng4 bxg4 (+3) *pxg4 (-3) -> 0");

  load_fen(&position, "rnbqkb1r/ppppp1pp/8/5p2/6n1/1P2P3/P1PPBPPP/RNBQK1NR",
           "w", "KQkq", "f6", "0", "4");
  TEST_ASSERT(see_after_move(&position, E2, G4, BISHOP) == 1,
              "*Ng4 bxg4 (+3) *fxg4 (-3) Qxg4 (+1) -> 1");

  load_fen(&position, "rnb1kb1r/pppp2pp/4p3/5pq1/6n1/1P2PP2/PBPPB1PP/RN1QK1NR",
           "w", "KQkq", "f6", "0", "4");
  TEST_ASSERT(
      see_after_move(&position, F3, G4, PAWN) == 3,
      "*Ng4 fxg4 (+3) *fxg4 (-1) bxg4 (+1) (*queen does not retaliate) -> 3");

  load_fen(&position,
           "rnb2b1r/pppp2pp/4p3/5pqk/P2P2n1/1PN1PP2/1BP1B1PP/R2QK1NR", "w",
           "KQkq", "-", "1", "9");
  TEST_ASSERT(
      see_after_move(&position, F3, G4, PAWN) == 3,
      "*Ng4 fxg4 (+3) *fxg4 (-1) bxg4 (+1) (*queen does not retaliate) -> 3");
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
