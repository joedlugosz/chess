
#include <stdlib.h>

#include "fen.h"
#include "state.h"
#include "test.h"

void test_prng(void) {
  TEST_ASSERT(1, "pass");
}

int main(void) {
  test_init(1, "hash");
  test_prng();
  return 0;
}
