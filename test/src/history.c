#include "history.h"

#include <stdio.h>
#include <stdlib.h>

#include "hash.h"
#include "io.h"
#include "position.h"
#include "test.h"

void test_history(void) {
  hash_init();
  tt_init();
  init_board();

  {
    struct move test_moves[] = {
        {B1, C3, 0, 0}, {B8, C6, 0, 0}, {C3, B1, 0, 0}, {C6, B8, 0, 0},
        {B1, C3, 0, 0}, {B8, C6, 0, 0}, {C3, B1, 0, 0}, {C6, B8, 0, 0},
        /* Starting position has been repeated 3 times */
    };
    struct history history;
    history_clear(&history);
    struct position position;
    reset_board(&position);
    int found_at = -1;
    for (int i = 0; i < sizeof(test_moves) / sizeof(test_moves[0]); i++) {
      history_push(&history, position.hash, &test_moves[i]);
      make_move(&position, &test_moves[i]);
      change_player(&position);
      if (is_repeated_position(&history, position.hash, 3)) {
        found_at = i;
      }
    }
    TEST_ASSERT(found_at == 7,
                "is_repeated_position detects threefold repetition from "
                "starting position");
  }
}

int main(void) {
  test_init(1, "history");
  test_history();
  return 0;
}
