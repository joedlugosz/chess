#ifndef TEST_H
#define TEST_H

#define TEST_ASSERT(pred, desc) {if (pred) test_pass(desc); else test_fail(desc);}

void test_init(int id, const char *_module);
void test_pass(const char *description);
void test_fail(const char *description);

#endif /* TEST_H */
