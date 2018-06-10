#include "dangless/virtmem.h"
#include "dangless/virtmem_alloc.h" // vp_reset()
#include "dangless/dangless_malloc.h"

#include "dunetest.h"

#define SYSMALLOC_HEADER_SIZE (0x10ul)
#define MALLOC_FILL_PG_SIZE ((size_t)PGSIZE - SYSMALLOC_HEADER_SIZE)

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

TEST_SUITE("Dune basics") {
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

    // Interesting, this assertion started failing randomly; looks like it won't always be the case that the memory is re-used.
    //ASSERT_EQUALS(pa1, pa2);

    TFREE(p2);
  }

  TEST("Simple calloc-free") {
    void *p = TCALLOC(64, void *);
    TFREE(p);
  }

  TEST("Page allocation") {
    void *p1 = TMALLOC(char[MALLOC_FILL_PG_SIZE]);

    // actually always going to be 2 virtual pages, due to the inpage-offset caused by the malloc header
    void *p2 = TMALLOC(char[PGSIZE]);

    TFREE(p2);
    TFREE(p1);
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
    // NOTE: a PGSIZE worth of malloc()-d area has to be placed on 2 virtual pages, due to the in-page offset: a malloc()-d area will never be on a page boundary, because the malloc header comes before it

    // this will allocate minimum 2 pages, but usually 3 pages, given that the allocation will probably start in the middle of a page
    void *p = TMALLOC(char[2 * MALLOC_FILL_PG_SIZE]);
    ASSERT_VALID_PTR((uint8_t *)p + 2 * MALLOC_FILL_PG_SIZE - 1);

    // this will definitely be minimum 1, maximum 2 pages, so we're shrinking the allocation by one page (at minimum)
    void *p2 = TREALLOC(p, char[MALLOC_FILL_PG_SIZE]);
    ASSERT_EQUALS(p, p2);

    // we're not guaranteed that a whole MALLOC_FILL_PG_SIZE worth of memory got invalidated, because we're not guaranteed to get a page-aligned region
    // however, what can definitely be guaranteed is that the last byte of the region will no longer be accessible
    ASSERT_INVALID_PTR((uint8_t *)p2 + 2 * MALLOC_FILL_PG_SIZE - 1);

    TFREE(p2);
  }

  TEST("realloc no growing in-place") {
    // doing it with just 2 * MALLOC_FILL_PG_SIZE wouldn't always work, since depending on how do in-page offsets work out, p1 and p2 might end up spanning the same pages
    void *p1 = TMALLOC(char[3 * MALLOC_FILL_PG_SIZE]);
    void *p2 = TREALLOC(p1, char[MALLOC_FILL_PG_SIZE]);
    ASSERT_EQUALS(p1, p2);

    // We cannot safely grow in-place, since we still might have a dangling pointer to p1 + 2 * MALLOC_FILL_PG_SIZE! This has to yield a new memory region.
    void *p3 = TREALLOC(p2, char[3 * MALLOC_FILL_PG_SIZE]);
    ASSERT_NOT_EQUALS(p1, p3);

    TFREE(p3);
  }

  TEST("realloc growing relocation") {
    // allocate two consecutive pages
    void *p1 = TMALLOC(char[MALLOC_FILL_PG_SIZE]);
    void *p2 = TMALLOC(char[MALLOC_FILL_PG_SIZE]);

    // note that the addresses won't be exactly equal, because there'll be a malloc header in between
    // we cannot guarantee they'll be directly after each other; glibc's malloc() for instance allocates somewhat more memory than strictly necessary, sometimes breaking this assumption when it falls across page boundaries
    //ASSERT_SAME_PAGE(PG_OFFSET(p1, 1), p2);

    // try to grow p1: this is not possible in-place because p2 is in the way
    void *p3 = TREALLOC(p1, char[3 * MALLOC_FILL_PG_SIZE]);
    ASSERT_NOT_EQUALS(p3, p1);
    ASSERT_NOT_EQUALS(p3, p2);

    TFREE(p3);
    TFREE(p2);
  }

  TEST("falling back to sysmalloc") {
    // reset the virtual page allocator, meaning there won't be any virtual pages available for dangless
    vp_reset();

    void *p = TMALLOC(void *);

    paddr_t pa = pt_resolve(p);
    ASSERT_NOT_EQUALS(pa, 0);

    ASSERT_EQUALS(pa, dune_va_to_pa(p));

    TFREE(p);
  }
}
