#include <stdio.h>

#include "ctestfx/ctestfx.h"

TEST_SUITE(metatest);

TEST_SUITE_SETUP(struct test_suite *tsuite) {
  fprintf(stderr, "Starting metatest suite...\n");
}

TEST_SUITE_TEARDOWN(struct test_suite *tsuite) {
  fprintf(stderr, "Metatest suite is over!\n");
}

TEST_CASE(always_pass) {
  /* empty */
}

TEST_CASE(never_pass) {
  EXPECT_NOT_NULL(NULL);
}

TEST_CASE(int_equality) {
  const int x = 43;
  EXPECT_EQUALS(x, 42);
}

TEST_CASE(str_equality) {
  const char *s = "potato";
  EXPECT_EQUALS_STR(s, "hello");
}
