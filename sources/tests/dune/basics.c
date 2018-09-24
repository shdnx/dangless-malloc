#include <stdlib.h>

#include "dangless/virtmem.h"
#include "dangless/virtmem_alloc.h" // vp_reset()
#include "dangless/dangless_malloc.h"

#include "dunetest.h"

TEST_SUITE(basics);

enum {
  SYSMALLOC_HEADER_SIZE = 0x10ul,
  MALLOC_FILL_PG_SIZE = ((size_t)PGSIZE - SYSMALLOC_HEADER_SIZE)
};

static bool find_free_vmemregion(/*OUT*/ void **pstart, /*OUT*/ void **pend) {
  // just find the first unused PML4 entry and returns the memregion for it
  const size_t pte_addr_mult = 1ul << pt_level_shift(PT_L4);
  pte_t *ptroot = pt_root();

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    if (ptroot[i])
      continue;

    /*OUT*/ *pstart = (void *)(i * pte_addr_mult);
    /*OUT*/ *pend = (void *)((i + 1) * pte_addr_mult);

    return true;
  }

  return false;
}

TEST_SUITE_SETUP() {
  // enter Dune, etc.
  dunetest_init();

  // dedicate virtual memory
  void *dvmem_start, *dvmem_end;
  if (!find_free_vmemregion(/*OUT*/ &dvmem_start, /*OUT*/ &dvmem_end)) {
    fprintf(stderr, "No free PML4 entry?!\n");
    abort();
  }

  vp_reset();
  dangless_dedicate_vmem(dvmem_start, dvmem_end);
}

TEST_CASE(free_null) {
  dangless_free(NULL); // should be a no-op per the standard
}

TEST_CASE(simple_malloc_free) {
  // this is basically TMALLOC(), used later
  void *p = dangless_malloc(sizeof(void *));
  EXPECT_VALID_ALLOC(p, sizeof(void *));

  paddr_t pa = pt_resolve(p);
  EXPECT_NOT_EQUALS(pa, 0);

  EXPECT_NOT_EQUALS((uintptr_t)p, (uintptr_t)pa);
  dangless_free(p);
}

TEST_CASE(simple_no_va_reuse) {
  void *p1 = TMALLOC(void *);
  paddr_t pa1 = pt_resolve(p1);
  LOGV("p1 = %p, pa1 = %p\n", p1, (void *)pa1);

  TFREE(p1);

  void *p2 = TMALLOC(void *);
  paddr_t pa2 = pt_resolve(p2);
  LOGV("p2 = %p, pa2 = %p\n", p2, (void *)pa2);

  EXPECT_NOT_EQUALS(p1, p2);

  // Interesting, this assertion started failing randomly; looks like it won't always be the case that the memory is re-used.
  //EXPECT_EQUALS(pa1, pa2);

  TFREE(p2);
}

TEST_CASE(simple_calloc_free) {
  void *p = TCALLOC(64, void *);
  TFREE(p);
}

TEST_CASE(page_alloc) {
  void *p1 = TMALLOC(char[MALLOC_FILL_PG_SIZE]);

  // actually always going to be 2 virtual pages, due to the inpage-offset caused by the malloc header
  void *p2 = TMALLOC(char[PGSIZE]);

  TFREE(p2);
  TFREE(p1);
}

TEST_CASE(many_small_allocs) {
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

TEST_CASE(realloc_null) {
  void *p = TREALLOC(NULL, void *);
  TFREE(p);
}

TEST_CASE(realloc_shrinking_inplace) {
  void *p = TMALLOC(char[128]);
  void *p2 = TREALLOC(p, char[64]);
  EXPECT_EQUALS(p, p2);

  TFREE(p2);
}

#define FILL_PAGES_SIZE(NPAGES) (MALLOC_FILL_PG_SIZE + (NPAGES - 1) * PGSIZE)

TEST_CASE(realloc_shrinking_pages) {
  // NOTE: a PGSIZE worth of malloc()-d area has to be placed on 2 virtual pages, due to the in-page offset: a malloc()-d area will never be on a page boundary, because the malloc header comes before it

  // this will allocate minimum 2 pages, but usually 3 pages, given that the allocation will probably start in the middle of a page
  void *p = TMALLOC(char[FILL_PAGES_SIZE(2)]);
  LOGV("p = %p\n", p);
  EXPECT_VALID_PTR((uint8_t *)p + FILL_PAGES_SIZE(2) - 1);

  // this will definitely be minimum 1, maximum 2 pages, so we're shrinking the allocation by one page (at minimum)
  void *p2 = TREALLOC(p, char[FILL_PAGES_SIZE(1)]);
  LOGV("p2 = %p\n", p2);
  EXPECT_EQUALS(p, p2);

  // we're not guaranteed that a whole MALLOC_FILL_PG_SIZE worth of memory got invalidated, because we're not guaranteed to get a page-aligned region
  // however, what can definitely be guaranteed is that the last byte of the region will no longer be accessible
  // TODO: something here with the math is incorrect
  LOGV("Has to be invalid: %p\n", (void *)((uint8_t *)p2 + PGSIZE - 1));
  EXPECT_INVALID_PTR((uint8_t *)p2 + PGSIZE - 1);

  TFREE(p2);
}

TEST_CASE(realloc_growing_inplace) {
  // doing it with just 2 * MALLOC_FILL_PG_SIZE wouldn't always work, since depending on how do in-page offsets work out, p1 and p2 might end up spanning the same pages
  void *p1 = TMALLOC(char[FILL_PAGES_SIZE(3)]);
  void *p2 = TREALLOC(p1, char[FILL_PAGES_SIZE(1)]);
  EXPECT_EQUALS(p1, p2);

  // We cannot safely grow in-place, since we still might have a dangling pointer to p1 + 2 * MALLOC_FILL_PG_SIZE! This has to yield a new memory region.
  void *p3 = TREALLOC(p2, char[FILL_PAGES_SIZE(3)]);
  EXPECT_NOT_EQUALS(p1, p3);

  TFREE(p3);
}

TEST_CASE(realloc_growing_reloc) {
  // allocate two consecutive pages
  void *p1 = TMALLOC(char[FILL_PAGES_SIZE(1)]);
  void *p2 = TMALLOC(char[FILL_PAGES_SIZE(1)]);

  // note that the addresses won't be exactly equal, because there'll be a malloc header in between
  // we cannot guarantee they'll be directly after each other; glibc's malloc() for instance allocates somewhat more memory than strictly necessary, sometimes breaking this assumption when it falls across page boundaries
  //EXPECT_SAME_PAGE(PG_OFFSET(p1, 1), p2);

  // try to grow p1: this is not possible in-place because p2 is in the way
  void *p3 = TREALLOC(p1, char[FILL_PAGES_SIZE(3)]);
  EXPECT_NOT_EQUALS(p3, p1);
  EXPECT_NOT_EQUALS(p3, p2);

  TFREE(p3);
  TFREE(p2);
}
