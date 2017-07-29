#include <stdio.h>
#include <string.h>

#include "printf_nomalloc.h"
#include "testfx.h"

TEST_SUITE("nomalloc_printf") {
  TEST("sprintf_nomalloc simple") {
    ASSERT_EQUALS_STR("42", sprintf_nomalloc("%d", 42));
    ASSERT_EQUALS_STR("-42", sprintf_nomalloc("%d", -42));
    ASSERT_EQUALS_STR("potato", sprintf_nomalloc("%s", "potato"));
    ASSERT_EQUALS_STR("q", sprintf_nomalloc("%c", 'q'));

    ASSERT_EQUALS_STR("0x0", sprintf_nomalloc("%p", NULL));
    ASSERT_EQUALS_STR("0x10FA74BC", sprintf_nomalloc("%p", (void *)0x10FA74BCul));
  }

  TEST("sprintf_nomalloc modifiers") {
    ASSERT_EQUALS_STR("99", sprintf_nomalloc("%ld", 99l));
    ASSERT_EQUALS_STR("578", sprintf_nomalloc("%lu", 578ul));
    ASSERT_EQUALS_STR("9897123", sprintf_nomalloc("%zu", (size_t)9897123));
  }

  TEST("sprintf_nomalloc complex") {
    ASSERT_EQUALS_STR("qxc", sprintf_nomalloc("%c%c%c", 'q', 'x', 'c'));
    ASSERT_EQUALS_STR("potatos", sprintf_nomalloc("%s%c", "potato", 's'));
    ASSERT_EQUALS_STR("potato92", sprintf_nomalloc("%s%d", "potato", 92));
    ASSERT_EQUALS_STR("42 0xFA potato", sprintf_nomalloc("%u 0x%x %s", 42, 0xFA, "potato"));
  }

  TEST("printing test") {
    fprintf_nomalloc(stdout, "stdout Hello world!\n");
    fprintf_nomalloc(stderr, "stderr Some numbers: %d %u\n", 42, 92u);
  }
}
