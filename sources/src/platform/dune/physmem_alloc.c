#include <string.h> // memset

#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/statistics.h"
#include "dangless/platform/physmem_alloc.h"

#include "dune.h"

#if DANGLESS_CONFIG_DEBUG_PHYSMEM_ALLOC
  #define LOG(...) vdprintf(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

STATISTIC_DEFINE_COUNTER(st_pp_allocations);
STATISTIC_DEFINE_COUNTER(st_pp_allocations_failed);
STATISTIC_DEFINE_COUNTER(st_pp_frees);

paddr_t pp_alloc(size_t npages) {
  ASSERT0(npages == 1);

  struct page *pg = dune_page_alloc();
  if (!pg) {
    STATISTIC_UPDATE() {
      st_pp_allocations_failed++;
    }

    LOG("Failed to allocate physical memory page!\n");
    return 0;
  }

  STATISTIC_UPDATE() {
    st_pp_allocations++;
  }

  paddr_t pa = dune_page2pa(pg);
  LOG("Allocated physical memory page: %p\n", (void *)pa);
  return pa;
}

paddr_t pp_zalloc(size_t npages) {
  paddr_t pa = pp_alloc(npages);
  if (!pa)
    return pa;

  // any phyical memory allocated by Dune's page allocator will be at PAGEBASE, which is identity-mapped
  // this is how Dune itself does it as well: alloc_page() in vm.c
  void *va = (void *)pa;
  memset(va, 0, npages * PGSIZE);

  return pa;
}

void pp_free(paddr_t pa, size_t npages) {
  ASSERT0(npages == 1);

  LOG("Freeing physical memory region from %p for %zu pages\n", (void *)pa, npages);
  struct page *pg = dune_pa2page(pa);
  ASSERT0(pg->ref == 1);

  STATISTIC_UPDATE() {
    st_pp_frees++;
  }

  dune_page_put(pg);
}
