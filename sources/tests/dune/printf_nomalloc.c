#include <stdio.h>
#include <string.h>

#include "dangless/common/printf_nomalloc.h"

#include "ctestfx/ctestfx.h"

TEST_SUITE(printf_nomalloc);

TEST_CASE(simple) {
  EXPECT_EQUALS_STR("42", sprintf_nomalloc("%d", 42));
  EXPECT_EQUALS_STR("-42", sprintf_nomalloc("%d", -42));
  EXPECT_EQUALS_STR("potato", sprintf_nomalloc("%s", "potato"));
  EXPECT_EQUALS_STR("q", sprintf_nomalloc("%c", 'q'));

  EXPECT_EQUALS_STR("0x0", sprintf_nomalloc("%p", NULL));
  EXPECT_EQUALS_STR("0x10FA74BC", sprintf_nomalloc("%p", (void *)0x10FA74BCul));
}

TEST_CASE(mod_intlen) {
  EXPECT_EQUALS_STR("99", sprintf_nomalloc("%ld", 99l));
  EXPECT_EQUALS_STR("578", sprintf_nomalloc("%lu", 578ul));
  EXPECT_EQUALS_STR("9897123", sprintf_nomalloc("%zu", (size_t)9897123));
}

TEST_CASE(mod_minfieldwidth) {
  EXPECT_EQUALS_STR("  xyz", sprintf_nomalloc("%5s", "xyz"));
  EXPECT_EQUALS_STR("xyz  ", sprintf_nomalloc("%-5s", "xyz"));
  EXPECT_EQUALS_STR("00042", sprintf_nomalloc("%05d", 42));
}

TEST_CASE(mod_minfieldwidth_custom) {
  EXPECT_EQUALS_STR("  xyz", sprintf_nomalloc("%*s", 5, "xyz"));
  EXPECT_EQUALS_STR("xyz  ", sprintf_nomalloc("%-*s", 5, "xyz"));
  EXPECT_EQUALS_STR("00042", sprintf_nomalloc("%0*d", 5, 42));
}

TEST_CASE(complex) {
  EXPECT_EQUALS_STR("qxc", sprintf_nomalloc("%c%c%c", 'q', 'x', 'c'));
  EXPECT_EQUALS_STR("potatos", sprintf_nomalloc("%s%c", "potato", 's'));
  EXPECT_EQUALS_STR("potato92", sprintf_nomalloc("%s%d", "potato", 92));
  EXPECT_EQUALS_STR("42 0xFA potato", sprintf_nomalloc("%u 0x%x %s", 42, 0xFA, "potato"));
}

TEST_CASE(printing) {
  fprintf_nomalloc(stdout, "stdout Hello world!\n");
  fprintf_nomalloc(stderr, "stderr Some numbers: %d %u\n", 42, 92u);
}
