#include "dangless/config.h"
#include "dangless/virtmem_alloc.h" // vp_reset()
#include "dangless/dangless_malloc.h"

#include "dunetest.h"

TEST_SUITE(sysmalloc_fallback);

#if DANGLESS_CONFIG_ALLOW_SYSMALLOC_FALLBACK

TEST_SUITE_SETUP() {
  dunetest_init();

  // reset the virtual page allocator, meaning there won't be any virtual pages available for dangless
  vp_reset();
}

TEST_CASE(1) {
  void *p = TMALLOC(void *);

  paddr_t pa = pt_resolve(p);
  EXPECT_NOT_EQUALS(pa, 0);

  LOGV("p = %p, pa = %p\n", p, (void *)pa);
  LOGV("dune_va_to_pa(p) = %p\n", (void *)dune_va_to_pa(p));

  EXPECT_EQUALS(pa, dune_va_to_pa(p));

  TFREE(p);
}

#endif
