#include <stdio.h>

#include "testfx.h"

TEST_SUITE("Metatest suite") {
  fprintf(stderr, "Starting metatest suite...\n");

  TEST("Always pass") {
    /* empty */
  }

  TEST("Never pass") {
    ASSERT_NOT_NULL(NULL);
  }

  TEST("Int equality") {
    int x = 43;
    ASSERT_EQUALS(x, 42);
  }

  TEST("String equality") {
    char *s = "potato";
    ASSERT_EQUALS(s, "hello");
  }
}
