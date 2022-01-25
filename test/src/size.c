
#include "state.h"

#define PRINT_SIZE(type) (printf("sizeof(%s) = %ld = %ld * 64\n", #type, \
  sizeof(type), (sizeof(type)-1)/64+1))

int main(void) {
  PRINT_SIZE(state_s);
  PRINT_SIZE(move_s);
  return 0;
}
