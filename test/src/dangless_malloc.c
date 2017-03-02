#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dangless_malloc.h"
#include "virtmem.h"
#include "virtmem_alloc.h"
#include "rumprun.h"

#define DGLMALLOC_DEBUG 1

#if DGLMALLOC_DEBUG
  #define DGLMALLOC_DPRINTF(...) dprintf("[dangless_malloc] " __VA_ARGS__)
#else
  #define DGLMALLOC_DPRINTF(...) /* empty */
#endif

// bmk-core/memalloc.h
enum {
  BMK_MEMALLOC_WHO = 2 // = BMK_MEMWHO_USER
};

RUMPRUN_DEF_FUNC(0xc440, void *, bmk_memalloc, unsigned long /*nbytes*/, unsigned long /*align*/, int /* enum bmk_memwho who */);
RUMPRUN_DEF_FUNC(0xc6b0, void, bmk_memfree, void * /* p */, int /* enum bmk_memwho who */);

enum {
  BMK_MEMALLOC_ALIGN = 16, // just to be safe
  DEAD_PTE = 0xDEAD00 // the PG_V bit must be 0 in this value!!!
};

static void *_memalloc(size_t sz) {
  return RUMPRUN_FUNC(bmk_memalloc)(sz, BMK_MEMALLOC_ALIGN, BMK_MEMALLOC_WHO);
}

static void _memfree(void *p) {
  RUMPRUN_FUNC(bmk_memfree)(p, BMK_MEMALLOC_WHO);
}

int dangless_dedicate_vmem(void *start, void *end) {
  DGLMALLOC_DPRINTF("dedicating virtual memory: %p - %p\n", start, end);
  return vp_free_region(start, end);
}

void *dangless_malloc(size_t sz) {
  void *p = _memalloc(sz);
  if (!p) {
    DGLMALLOC_DPRINTF("dangless_malloc failed: bmk_memalloc() returned NULL!\n");
    return NULL;
  }

  void *va = vp_alloc_one();
  if (!va) {
    DGLMALLOC_DPRINTF("dangless_malloc: could not allocate virtual memory page, falling back to just proxying bmk_memalloc\n");
    return p;
  }

#if DGLMALLOC_DEBUG
  {
    enum pt_level level;
    paddr_t pa = get_paddr_page(va, OUT &level);
    assert(!pa && "Allocated virtual page is already mapped to a physical address!");
  }
#endif

  paddr_t pa = ROUND_DOWN((paddr_t)p, PGSIZE);
  int result;
  if ((result = pt_map(pa, (vaddr_t)va, PG_RW | PG_NX)) < 0) {
    DGLMALLOC_DPRINTF("dangless_malloc failed: could not map pa 0x%lx to va %p, code %d; falling back to proxying bmk_memalloc\n", pa, va, result);

    // try to give back the virtual memory page - this may fail, but we can't do anything about it
    vp_free_one(va);
    return p;
  }

  return (uint8_t *)va + get_page_offset((uintptr_t)p, PT_4K);
}

void dangless_free(void *p) {
  // compatibility with the standard free()
  if (!p)
    return;

  pte_t *ppte;
  enum pt_level level = pt_walk(p, PGWALK_FULL, OUT &ppte);
  assert(FLAG_ISSET(*ppte, PG_V));

  if (level == PT_L1) {
    paddr_t pa = (paddr_t)(*ppte & PG_FRAME);
    pa += get_page_offset((uintptr_t)p, PT_L1);

    *ppte = DEAD_PTE;

    // since the original virtual address == physical address, we can just get the physical address and give that to bmk_memfree()
    // this allows us to get away with not maintaining mappings from the remapped virtual addresses to the original ones
    _memfree((void *)pa);
  } else {
    // we must have failed to allocate a dedicated virtual page, just forward to the bmk_memfree()
    _memfree(p);
  }
}