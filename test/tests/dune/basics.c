#include <stdlib.h>

// TODO: dangless prefix
// TODO: also create a header file for distribution, that doesn't expose e.g. common.h
#include "virtmem.h"
#include "dangless_malloc.h"
#include "platform/sysmalloc.h"

#include "testfx.h"

#include "libdune/dune.h"

#define TMALLOC(TYPE) \
  ({ \
    void *tmalloc_ptr = malloc(sizeof(TYPE)); \
    ASSERT_NOT_NULL(tmalloc_ptr); \
    tmalloc_ptr; \
  })

TEST_SUITE("Dune basics") {
  fprintf(stderr, "Entering Dune...\n");

  if (!dune_init_and_enter()) {
    // TODO: a more elegant solution to this
    fprintf(stderr, "Failed to enter Dune mode!\n");
    abort();
  }

  fprintf(stderr, "Now in Dune, starting test suite...\n");

  TEST("Dune is in kernel mode") {
    ASSERT_TRUE(in_kernel_mode());
  }

  TEST("Symbol override") {
    ASSERT_EQUALS_PTR(&malloc, &dangless_malloc);
    ASSERT_EQUALS_PTR(&calloc, &dangless_calloc);
    ASSERT_EQUALS_PTR(&free, &dangless_free);
  }

  TEST("Simple malloc-free") {
    void *p = malloc(sizeof(void *));
    ASSERT_NOT_NULL(p);

    paddr_t pa = pt_resolve(p);
    ASSERT_NOT_EQUALS(pa, 0);

    ASSERT_NOT_EQUALS((uintptr_t)p, (uintptr_t)pa);
    free(p);
  }

  TEST("Simple no va-reuse") {
    void *p1 = TMALLOC(void *);
    paddr_t pa1 = pt_resolve(p1);

    free(p1);

    void *p2 = TMALLOC(void *);
    paddr_t pa2 = pt_resolve(p2);

    ASSERT_NOT_EQUALS(p1, p2);
    ASSERT_EQUALS(pa1, pa2);

    free(p2);
  }
}
