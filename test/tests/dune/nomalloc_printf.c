#include <stdio.h>
#include <string.h>

#include "nomalloc_printf.h"
#include "testfx.h"

TEST_SUITE("nomalloc_printf") {
  TEST("nomalloc_sprintf simple") {
    ASSERT_EQUALS_STR("42", nomalloc_sprintf("%d", 42));
    ASSERT_EQUALS_STR("-42", nomalloc_sprintf("%d", -42));
    ASSERT_EQUALS_STR("potato", nomalloc_sprintf("%s", "potato"));
    ASSERT_EQUALS_STR("q", nomalloc_sprintf("%c", 'q'));

    ASSERT_EQUALS_STR("0x0", nomalloc_sprintf("%p", 0));
    ASSERT_EQUALS_STR("0x10FA74BC", nomalloc_sprintf("%p", 0x10FA74BC));
  }

  TEST("nomalloc_sprintf complex") {
    ASSERT_EQUALS_STR("qxc", nomalloc_sprintf("%c%c%c", 'q', 'x', 'c'));
    ASSERT_EQUALS_STR("potatos", nomalloc_sprintf("%s%c", "potato", 's'));
    ASSERT_EQUALS_STR("potato92", nomalloc_sprintf("%s%d", "potato", 92));
    ASSERT_EQUALS_STR("42 0xFA potato", nomalloc_sprintf("%u %x %s", 42, 0xFA, "potato"));
  }

  TEST("printing test") {
    nomalloc_fprintf(stdout, "stdout Hello world!\n");
    nomalloc_fprintf(stderr, "stderr Some numbers: %d %u\n", 42, 92u);
  }
}
