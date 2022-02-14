#include "hash.h"

#include <stdio.h>
#include <stdlib.h>

#include "fen.h"
#include "state.h"
#include "test.h"

void test_prng(void) {
  prng_seed(0);

  hash_t val = prng_rand();
  TEST_ASSERT(prng_rand() != val, "Consecutive values from the same seed are different");

  prng_seed(0);
  TEST_ASSERT(prng_rand() == val, "Seed produces the same initial value");

  /* All keys must be unique */
  prng_seed(0);
  hash_init();
  int collision = 0, i, j, k, l;
  for (i = 0; i < N_PLANES && !collision; i++) {
    for (j = 0; j < N_SQUARES && !collision; j++) {
      for (k = 0; k < N_PLANES && !collision; k++) {
        for (l = 0; l < N_SQUARES && !collision; l++) {
          if (i == k && j == l) continue;
          if (placement_key[i][j] == placement_key[k][l]) {
            collision = 1;
          }
        }
      }
    }
  }
  char buf[100];
  buf[0] = 0;
  if (collision) {
    sprintf(buf, "placment_key[%d][%d] != placement_key[%d][%d]", i, j, k, l);
  }
  TEST_ASSERT(!collision, buf);
}
/*
void test_hash_state(state_s *state) {
  char buf[1000];
  char fen[100];
  get_fen(state, fen, sizeof(fen));

  state_s state_b;
  load_fen(&state_b, fen);

  sprintf(buf, "Hash can be re-created FEN: %s\n", fen);
  TEST_ASSERT(state->hash == state_b.hash, buf);

  sprintf(buf, "State can be re-created FEN: %s\n", fen);
  TEST_ASSERT(memcmp(state, &state_b, sizeof(state)) == 0, buf);
}
*/
int main(void) {
  test_init(1, "hash");
  test_prng();
  return 0;
}
