#include <stdio.h>
#include <stdlib.h>

int test = 0;
const char *module = 0;

void test_init(int id, const char *_module) {
  module = _module;
  test = id;
}

void test_pass(const char *description) {
  printf("Passed %s-%d - %s\n", module, test, description);
  test++;
}

void test_fail(const char *description) {
  printf("Failed %s-%d - %s\n", module, test, description);
  exit(1);
}
