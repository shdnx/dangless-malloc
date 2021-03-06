#include <stdio.h>
#include <stdlib.h> // abort()

#include <dangless/virtmem.h>
#include <dangless/virtmem_alloc.h>

#include <ctestfx/ctestfx.h>

// TODO: we should enforce that this test suite runs before e.g. 'basics.c'
TEST_SUITE(vpalloc);

TEST_SUITE_SETUP() {
  vp_reset();

  // &PML4[2], so dangless' auto-dedication will only take PML4[1]
  void *const region_start = (void *)(2ul << pt_level_shift(PT_L4));
  const size_t region_npages = pt_num_mapped_pages(PT_L4);

  // TODO: this testing framework needs some "setup assert"
  if (vp_free(region_start, region_npages) != 0) {
    fprintf(stderr, "Failed to free virtual region at %p for %zu pages!\n", region_start, region_npages);
    abort();
  }
}

/*TEST_CASE(simple_onepage) {
  // allocate two pages, they should be the first two pages of the region
  void *p1, *p2;
  EXPECT_EQUALS(region_start, p1 = vp_alloc(1));
  EXPECT_EQUALS(PG_OFFSET(region_start, 1), p2 = vp_alloc(1));

  // free both
  EXPECT_EQUALS(vp_free(p1, 1), 0);
  EXPECT_EQUALS(vp_free(p2, 1), 0);

  // now allocate 2 pages at once - we should again get back region_start due to span merging
  void *p3;
  EXPECT_EQUALS(region_start, p3 = vp_alloc(2));
  EXPECT_EQUALS(vp_free(p3, 2), 0);
}*/
