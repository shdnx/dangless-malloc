#include <string.h> // memset()
#include <assert.h>

#include "queue.h"
#include "virtmem.h"

#include "platform/physmem_alloc.h"
#include "platform/sysmalloc.h"

#include "rumprun.h"

// Physical page allocation with rumprun is tricky. Rumprun appears to use a virtual memory identity mapping for the first 4 GBs of the address space, and it doesn't use the rest.
// This means that it doesn't have a physical memory allocator. It only has a page allocator in bmk-core/pgalloc.h.
// However, because all virtual memory mappings use 2 MB hugepages, we cannot actually use this directly to get 4K physical pages.
// Instead, we allocate a 512 pages via rumprun's page allocator, so a 2 MB physical hugepage. We can then carve this up into 4K pages.

// rumprun/include/bmk-core/pgalloc.h
RUMPRUN_DEF_FUNC(0xce60, void *, bmk_pgalloc, int /* order */);
RUMPRUN_DEF_FUNC(0xcc70, void *, bmk_pgalloc_align, int /* order */, unsigned long /*align*/);
RUMPRUN_DEF_FUNC(0xce70, void, bmk_pgfree, void * /* ptr */, int /* order */);

#if PHYSMEM_DEBUG
  #define PHYSMEM_DPRINTF(...) dprintf("[physmem] " __VA_ARGS__)
#else
  #define PHYSMEM_DPRINTF(...) /* empty */
#endif

#define BMK_PGALLOC_ORDER 9 // 512 pages, i.e. 1x 2MB hugepage
#define BMK_PGALLOC_NPAGES (1uL << BMK_PGALLOC_ORDER)
#define BMK_PGALLOC_ALIGN (BMK_PGALLOC_NPAGES * PGSIZE)

// Physical pages are identified by their index.
typedef uint32_t pp_index_t;

#define pa2ppindex(PA) ((pp_index_t)((PA) / PGSIZE))
#define ppindex2pa(IDX) ((paddr_t)((IDX) * PGSIZE))

// TODO: currently we never give back the physical memory to the page allocator
// TODO: if we ever want to support a more general use-case, i.e. pages that might be allocated/freed often, then we should consider having a pp_span represent a 2 MB space, and track the used pages via a bitmap.

// A span of free 4K physical pages.
struct pp_span {
  pp_index_t begin;
  pp_index_t end;

  LIST_ENTRY(pp_span) freelist;
};

// A freelist of pp_span objects, not ordered in any way.
static LIST_HEAD(, pp_span) ppspan_freelist = LIST_HEAD_INITIALIZER(&ppspan_freelist);

static struct pp_span *physmem_acquire(void) {
  void *p = RUMPRUN_FUNC(bmk_pgalloc_align)(BMK_PGALLOC_ORDER, BMK_PGALLOC_ALIGN);
  if (!p) {
    PHYSMEM_DPRINTF("bmk_pgalloc_align() failed: out of memory?\n");
    return NULL;
  }

  enum pt_level level;
  paddr_t pa = pt_resolve_page(p, OUT &level);
  assert(pa && "bmk_pgalloc_align() returned a page not backed by physical memory!");
  assert(level == PT_2M && "bmk_pgalloc_align() returned virtual memory not backed by a 2M hugepage!");

  struct pp_span *ps = MALLOC(struct pp_span);
  if (!ps) {
    PHYSMEM_DPRINTF("Failed to malloc pp_span: out of memory?\n");
    RUMPRUN_FUNC(bmk_pgfree)(p, BMK_PGALLOC_ORDER);
    return NULL;
  }

  ps->begin = pa2ppindex(pa);
  ps->end = ps->begin + BMK_PGALLOC_NPAGES;
  PHYSMEM_DPRINTF("Acquired physical memory pages: 0x%x - 0x%x (%lu pages) => pp_span %p\n", ps->begin, ps->end, BMK_PGALLOC_NPAGES, ps);

  LIST_INSERT_HEAD(&ppspan_freelist, ps, freelist);
  return ps;
}

static void physmem_free(struct pp_span *ps) {
  assert(ps->begin % (BMK_PGALLOC_ALIGN / PGSIZE) == 0 && "pp_span doesn't begin at an alignment boundary!");
  assert((ps->end - ps->begin) == BMK_PGALLOC_NPAGES && "pp_span is incomplete!");

  PHYSMEM_DPRINTF("Freeing physical memory pages 0x%x - 0x%x, pp_span %p\n", ps->begin, ps->end, ps);

  // NOTE: using identity-mapping
  void *p = (void *)(ppindex2pa(ps->begin));
  RUMPRUN_FUNC(bmk_pgfree)(p, BMK_PGALLOC_ORDER);

  LIST_REMOVE(ps, freelist);
  FREE(ps);
}

paddr_t pp_alloc(size_t npages) {
  // cannot use this to allocate more than 512 pages, i.e. 2 MB
  assert(npages < BMK_PGALLOC_NPAGES);

  struct pp_span *ps;
  LIST_FOREACH(ps, &ppspan_freelist, freelist) {
    if (ps->end - ps->begin >= npages)
      break;
  }

  if (ps == LIST_END(&ppspan_freelist)) {
    PHYSMEM_DPRINTF("No free pp_span containing %zu pages: attempt to acquire physical memory\n", npages);

    ps = physmem_acquire();
    if (!ps) {
      PHYSMEM_DPRINTF("pp_alloc failed: failed to acquire physical memory\n");
      return 0;
    }
  }

  assert(ps);
  paddr_t pa = ppindex2pa(ps->begin);
  ps->begin += npages;

  PHYSMEM_DPRINTF("Allocating %zu pages from pp_span %p => 0x%lx\n", npages, ps, pa);

  if (ps->begin == ps->end) {
    PHYSMEM_DPRINTF("pp_span %p is now empty, deallocating\n", ps);
    LIST_REMOVE(ps, freelist);
    FREE(ps);
  }

  return pa;
}

paddr_t pp_zalloc(size_t npages) {
  paddr_t pa = pp_alloc(npages);
  if (!pa)
    return 0;

  // NOTE: assumes identity-mapping
  void *va = (void *)pa;
  memset(va, 0, npages * PGSIZE);

  return pa;
}

void pp_free(paddr_t pa, size_t npages) {
  assert(pa % PGSIZE == 0);
  assert(0 < npages && npages < BMK_PGALLOC_NPAGES);

  // TODO: it's a paradox situation that we need to allocate memory in order to free memory, and it's not very nice, should be fixed
  // it's okay for now, since memory allocated via this module will very often never be freed
  struct pp_span *ps = MALLOC(struct pp_span);
  assert(ps);

  ps->begin = pa2ppindex(pa);
  ps->end = ps->begin + npages;
  LIST_INSERT_HEAD(&ppspan_freelist, ps, freelist);
}