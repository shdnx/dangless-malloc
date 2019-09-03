#include <stdlib.h>

#include <dangless/virtmem.h>
#include <dangless/dangless_malloc.h>
#include <dangless/config.h>

#include "dunetest.h"

TEST_SUITE(dune_common);

TEST_SUITE_SETUP() {
  dunetest_init();
}

TEST_CASE(kernel_mode) {
  EXPECT_TRUE(in_kernel_mode());
}

TEST_CASE(sym_override) {
  #if DANGLESS_CONFIG_OVERRIDE_SYMBOLS
    EXPECT_EQUALS_PTR(&malloc, &dangless_malloc);
    EXPECT_EQUALS_PTR(&calloc, &dangless_calloc);
    EXPECT_EQUALS_PTR(&free, &dangless_free);
  #else
    EXPECT_NOT_EQUALS_PTR(&malloc, &dangless_malloc);
    EXPECT_NOT_EQUALS_PTR(&calloc, &dangless_calloc);
    EXPECT_NOT_EQUALS_PTR(&free, &dangless_free);
  #endif
}
