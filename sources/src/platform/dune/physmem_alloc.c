#include <string.h> // memset

#include "platform/physmem_alloc.h"

#include "dune.h"

paddr_t pp_alloc(size_t npages) {
  ASSERT0(npages == 1);

  struct page *pg = dune_page_alloc();
  if (!pg)
    return 0;

  return dune_page2pa(pg);
}

paddr_t pp_zalloc(size_t npages) {
  paddr_t pa = pp_alloc(npages);
  if (!pa)
    return pa;

  // any phyical memory allocated by Dune's page allocator will be at PAGEBASE, which is identity-mapped
  void *va = (void *)pa;
  memset(va, 0, npages * PGSIZE);

  return pa;
}

void pp_free(paddr_t pa, size_t npages) {
  ASSERT0(npages == 1);

  struct page *pg = dune_pa2page(pa);
  ASSERT0(pg->ref == 1);

  dune_page_put(pg);
}