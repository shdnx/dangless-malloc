#include "virtmem.h"
#include "dangless_malloc.h"
#include "dangless/buildconfig.h"

#include "dunetest.h"

TEST_SUITE("Dune common") {
  dunetest_init();

  TEST("Dune is in kernel mode") {
    ASSERT_TRUE(in_kernel_mode());
  }

#if DANGLESS_CONFIG_OVERRIDE_SYMBOLS
  TEST("Symbol override") {
    ASSERT_EQUALS_PTR(&malloc, &dangless_malloc);
    ASSERT_EQUALS_PTR(&calloc, &dangless_calloc);
    ASSERT_EQUALS_PTR(&free, &dangless_free);
  }
#else
  TEST("Symbol override inactive") {
    ASSERT_NOT_EQUALS_PTR(&malloc, &dangless_malloc);
    ASSERT_NOT_EQUALS_PTR(&calloc, &dangless_calloc);
    ASSERT_NOT_EQUALS_PTR(&free, &dangless_free);
  }
#endif
}
