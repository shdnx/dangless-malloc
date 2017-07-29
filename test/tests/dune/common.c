#include "virtmem.h"
#include "dangless_malloc.h"

#include "dunetest.h"

TEST_SUITE("Dune common") {
  dunetest_init();

  TEST("Dune is in kernel mode") {
    ASSERT_TRUE(in_kernel_mode());
  }

  TEST("Symbol override") {
    ASSERT_EQUALS_PTR(&malloc, &dangless_malloc);
    ASSERT_EQUALS_PTR(&calloc, &dangless_calloc);
    ASSERT_EQUALS_PTR(&free, &dangless_free);
  }
}