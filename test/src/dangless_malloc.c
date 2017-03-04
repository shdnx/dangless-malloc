#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dangless_malloc.h"
#include "virtmem.h"
#include "virtmem_alloc.h"
#include "sysmalloc.h"

#define DGLMALLOC_DEBUG 1

#if DGLMALLOC_DEBUG
  #define DGLMALLOC_DPRINTF(...) dprintf("[dangless_malloc] " __func__ ": " __VA_ARGS__)
#else
  #define DGLMALLOC_DPRINTF(...) /* empty */
#endif

enum {
  DEAD_PTE = 0xDEAD00 // the PG_V bit must be 0 in this value!!!
};

int dangless_dedicate_vmem(void *start, void *end) {
  DGLMALLOC_DPRINTF("dedicating virtual memory: %p - %p\n", start, end);
  return vp_free_region(start, end);
}

static void *virtual_remap(void *p, size_t size) {
  assert(p);

  void *va = vp_alloc_one();
  if (!va) {
    DGLMALLOC_DPRINTF("could not allocate virtual memory page, falling back to just proxying bmk_memalloc\n");
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
  if ((result = pt_map_region(pa, (vaddr_t)va, ROUND_UP(size, PGSIZE), PG_RW | PG_NX)) < 0) {
    DGLMALLOC_DPRINTF("could not map pa 0x%lx to va %p, code %d; falling back to proxying bmk_memalloc\n", pa, va, result);

    // try to give back the virtual memory page - this may fail, but we can't do anything about it
    vp_free_one(va);
    return p;
  }

  return (uint8_t *)va + get_page_offset((uintptr_t)p, PT_4K);
}

// For a BMK malloc(), calloc(), etc. allocated chunk, return the number of 4K pages it spans (minimum 1).
// TODO: this is very bad, very tight coupling with the internal implementation details of BMK's memalloc.c - however, the only alternative is adding our own header, with its own overhead...
static size_t get_bmk_malloc_chunk_npages(void *p) {
/*
// bmk-core/memalloc.c:
struct memalloc_hdr {
  uint32_t  mh_alignpad; // padding for alignment
  uint16_t  mh_magic;    // magic number
  uint8_t   mh_index;    // bucket number
  uint8_t   mh_who;      // who allocated
};

// ...

#define MINSHIFT 5
#define LOCALBUCKETS (BMK_PCPU_PAGE_SHIFT - MINSHIFT)

// ...

  if (bucket >= LOCALBUCKETS) {
    hdr = bmk_pgalloc(bucket+MINSHIFT - BMK_PCPU_PAGE_SHIFT);
  } else {
    hdr = bucketalloc(bucket);
  }

// bmk-core/pgalloc.c:
#define order2size(_order_) (1UL<<(_order_ + BMK_PCPU_PAGE_SHIFT))
 */

#define BMK_MEMALLOC_LOCALBUCKETS 7

  uint8_t bucket_index = *((uint8_t *)p - 2);
  if (bucket_index <= BMK_MEMALLOC_LOCALBUCKETS)
    return 1;

  int pgalloc_order = bucket_index - BMK_MEMALLOC_LOCALBUCKETS;
  return (size_t)(1uL << pgalloc_order);
}

void *dangless_malloc(size_t size) {
  void *p = sysmalloc(size);
  if (!p) {
    DGLMALLOC_DPRINTF("bmk_memalloc() returned NULL!\n");
    return NULL;
  }

  return virtual_remap(p, size);
}

void *dangless_calloc(size_t num, size_t size) {
  void *p = syscalloc(num, size);
  if (!p) {
    DGLMALLOC_DPRINTF("bmk_memcalloc() returned NULL!\n");
    return NULL;
  }

  return virtual_remap(p, size);
}

void *dangless_realloc(void *p, size_t new_size) {
  void *newp = sysrealloc(p, new_size);
  if (!newp)
    return NULL;


}

int dangless_posix_memalign(void **pp, size_t align, size_t size) {
  int result = sysmemalign(pp, align, size);
  if (result != 0)
    return result;

  *pp = virtual_remap(*pp, size);
  return 0;
}

static bool virtual_invalidate(void *p, size_t npages, OUT void **original_ptr) {
  pte_t *ppte;
  enum pt_level level = pt_walk(p, PGWALK_FULL, OUT &ppte);
  assert(FLAG_ISSET(*ppte, PG_V));

  // currently this is only possible when we failed to allocate a dedicated virtual page for this allocation, and just falled back to proxying sysmalloc()
  if (level != PT_L1) {
    OUT *original_ptr = p;
    return false;
  }

  // since the original virtual address == physical address, we can just get the physical address and pretend it's a virtual address: specifically, it's the original virtual address, before we remapped it with virtual_remap()
  // this trickery allows us to get away with not maintaining mappings from the remapped virtual addresses to the original ones
  paddr_t pa = (paddr_t)(*ppte & PG_FRAME) + get_page_offset((uintptr_t)p, PT_L1);
  OUT *original_ptr = (void *)pa;

  // now invalidate the virtual pages: we do this by overwriting the PTEs with a magic number, taking care that the PG_V bit is 0... this allows us to detect a dangling pointer attempted access if we hook into the pagefault interrupt
  *ppte = DEAD_PTE;

  size_t page;
  for (page = 1; page < npages; page++) {
    // TODO: this could be optimised, since usually we'll be touching adjecent PTEs
    level = pt_walk((uint8_t *)p + (page * PGSIZE), PGWALK_FULL, OUT &ppte);
    assert(level == PT_L1 && FLAG_ISSET(*ppte, PG_V));

    *ppte = DEAD_PTE;
  }

  return true;
}

void dangless_free(void *p) {
  // compatibility with the libc free()
  if (!p)
    return;

  size_t npages = get_bmk_malloc_chunk_npages(p);

  void *original_ptr;
  virtual_invalidate(p, npages, OUT &original_ptr);

  sysfree(original_ptr);
}

// strong overrides of the libc memory allocation symbols
// when this all gets moved to a library, --whole-archive will have to be used when linking against the library so that these symbols will get picked up instead of the libc one
__strong_alias(malloc, dangless_malloc);
__strong_alias(calloc, dangless_calloc);
__strong_alias(realloc, dangless_realloc);
__strong_alias(posix_memalign, dangless_posix_memalign);
__strong_alias(free, dangless_free);