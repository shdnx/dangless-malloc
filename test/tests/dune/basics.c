// TODO: dangless prefix
// TODO: also create a header file for distribution, that doesn't expose e.g. common.h
#include "virtmem.h"
#include "virtmem_alloc.h" // vp_reset()
#include "dangless_malloc.h"

#include "dunetest.h"

TEST_SUITE("Dune basics") {
  // enter Dune, etc.
  dunetest_init();

  TEST_SETUP() {
    // reset the virtual page allocator
    vp_reset();

    // dedicate PML4[2]
    // TODO
  }

#if 0
  TEST("free(NULL)") {
    dangless_free(NULL);
  }

  TEST("Simple malloc-free") {
    // this is basically TMALLOC(), used later
    void *p = dangless_malloc(sizeof(void *));
    ASSERT_VALID_ALLOC(p, sizeof(void *));

    paddr_t pa = pt_resolve(p);
    ASSERT_NOT_EQUALS(pa, 0);

    ASSERT_NOT_EQUALS((uintptr_t)p, (uintptr_t)pa);
    dangless_free(p);
  }

  TEST("Simple no va-reuse") {
    void *p1 = TMALLOC(void *);
    paddr_t pa1 = pt_resolve(p1);

    TFREE(p1);

    void *p2 = TMALLOC(void *);
    paddr_t pa2 = pt_resolve(p2);

    ASSERT_NOT_EQUALS(p1, p2);
    ASSERT_EQUALS(pa1, pa2);

    TFREE(p2);
  }

  TEST("Simple calloc-free") {
    void *p = TCALLOC(64, void *);
    TFREE(p);
  }

  TEST("Page allocation") {
    void *p = TMALLOC(char[PGSIZE]);
    TFREE(p);
  }

  TEST("Many small allocations") {
    const size_t N = 1000;

    size_t i;
    void *prev = NULL;
    for (i = 0; i < N; i++) {
      void **p = TMALLOC(void **);
      *p = prev;

      prev = p;
    }

    void *curr, *next;
    for (curr = prev; curr; curr = next) {
      next = *(void **)curr;
      TFREE(curr);
    }
  }
#endif

  TEST("realloc(NULL)") {
    void *p = TREALLOC(NULL, void *);
    TFREE(p);
  }

  TEST("realloc shrinking in-place") {
    void *p = TMALLOC(char[128]);
    void *p2 = TREALLOC(p, char[64]);
    ASSERT_EQUALS(p, p2);

    TFREE(p2);
  }

  TEST("realloc shrinking pages") {
    void *p = TMALLOC(char[2 * PGSIZE]);
    ASSERT_VALID_PTR((uint8_t *)p + 2 * PGSIZE - 1);

    void *p2 = TREALLOC(p, char[PGSIZE]);
    ASSERT_EQUALS(p, p2);

    // we're not guaranteed that a whole PGSIZE worth of memory got invalidated, because we're not guaranteed to get a page-aligned region
    // however, what can definitely be guaranteed is that the last byte of the region will no longer be accessible
    ASSERT_INVALID_PTR((uint8_t *)p2 + 2 * PGSIZE - 1);

    TFREE(p2);
  }

  TEST("realloc growing in-place") {
    // this is guaranteed to be OK due to the allocation patterns we have before this unit test - a more solid solution would be to malloc 2 * PGSIZE, resize it to PGSIZE, and then resize it back to 2 * PGSIZE
    void *p1 = TMALLOC(char[2 * PGSIZE]);
    void *p2 = TREALLOC(p1, char[PGSIZE]);
    ASSERT_EQUALS(p1, p2);

    void *p3 = TREALLOC(p2, char[2 * PGSIZE]);
    ASSERT_EQUALS(p1, p3);

    TFREE(p3);
  }

  TEST("realloc growing relocation") {
    // allocate two pages: they should be consecutive due to the previous tests
    void *p1 = TMALLOC(char[PGSIZE]);
    void *p2 = TMALLOC(char[PGSIZE]);
    ASSERT_EQUALS(PG_OFFSET(p1, 1), p2);

    // try to grow p1: this is not possible in-place because p2 is in the way
    void *p3 = TREALLOC(p1, char[2 * PGSIZE]);
    ASSERT_NOT_EQUALS(p3, p1);
    ASSERT_NOT_EQUALS(p3, p2);

    TFREE(p3);
  }
}
