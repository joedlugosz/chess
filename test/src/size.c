
#include <stdio.h>

#include "position.h"
#define PRINT_SIZE(type)                                        \
  (printf("sizeof(%s) = %ld = %ld * 64\n", #type, sizeof(type), \
          (sizeof(type) - 1) / 64 + 1))

int main(void) {
  PRINT_SIZE(struct position);
  PRINT_SIZE(struct move);
  return 0;
}
