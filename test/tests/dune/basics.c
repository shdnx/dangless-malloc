// TODO: dangless prefix
// TODO: also create a header file for distribution, that doesn't expose e.g. common.h
#include "virtmem.h"
#include "dangless_malloc.h"

#include "dunetest.h"

TEST_SUITE("Dune basics") {
  dunetest_init();

  TEST("free(NULL)") {
    free(NULL);
  }

  TEST("Simple malloc-free") {
    // this is basically TMALLOC(), used later
    void *p = malloc(sizeof(void *));
    ASSERT_VALID_ALLOC(p, sizeof(void *));

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

  TEST("Simple calloc-free") {
    void *p = TCALLOC(64, void *);
    free(p);
  }

  TEST("Page allocation") {
    void *p = TMALLOC(char[PGSIZE]);
    free(p);
  }

  TEST("Many small allocations") {
    const size_t N = 10000;

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
      free(curr);
    }
  }
}
