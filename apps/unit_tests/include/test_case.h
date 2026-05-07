// a test suite function returns pass/fail
#define DEFINE_TEST_SUITE(name) extern int test_##name();

#define TEST_SUITE(name) int test_##name()

#define RUN_TEST_SUITE(name) test_##name();

// a test case returns pass/fail
#define TEST_CASE(name) int test_##name()

#define RUN_TEST_CASE(name) pass &= test_##name();