#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "dangless_malloc.h"
#include "virtmem.h"
#include "virtmem_alloc.h"

#include "platform/sysmalloc.h"

#define DGLMALLOC_DEBUG 1

#if DGLMALLOC_DEBUG
  #define DPRINTF(...) vdprintf(__VA_ARGS__)
#else
  #define DPRINTF(...) /* empty */
#endif

enum {
  DEAD_PTE = 0xDEAD00 // the PG_V bit must be 0 in this value!!!
};

STATIC_ASSERT(!FLAG_ISSET(DEAD_PTE, PTE_V), "DEAD_PTE cannot be a valid PTE!!!");

int dangless_dedicate_vmem(void *start, void *end) {
  DPRINTF("dedicating virtual memory: %p - %p\n", start, end);
  return vp_free_region(start, end);
}

static void *virtual_remap(void *p, size_t size) {
  assert(p);

  void *va = vp_alloc_one();
  if (!va) {
    DPRINTF("could not allocate virtual memory page, falling back to just proxying bmk_memalloc\n");
    return p;
  }

#if DGLMALLOC_DEBUG
  {
    enum pt_level level;
    paddr_t pa = pt_resolve_page(va, OUT &level);
    assert(!pa && "Allocated virtual page is already mapped to a physical address!");
  }
#endif

  paddr_t pa = vaddr2paddr(p);
  paddr_t pa_page = ROUND_DOWN(pa, PGSIZE);
  int result;
  if ((result = pt_map_region(pa_page, (vaddr_t)va, ROUND_UP(size, PGSIZE), PTE_W | PTE_NX)) < 0) {
    DPRINTF("could not map pa 0x%lx to va %p, code %d; falling back to proxying bmk_memalloc\n", pa_page, va, result);

    // try to give back the virtual memory page - this may fail, but we can't do anything about it
    vp_free_one(va);
    return p;
  }

  return (uint8_t *)va + get_page_offset((uintptr_t)p, PT_4K);
}

// TODO: this function is very rumprun-specific
// how could we make this platform-independent? when physical address != original virtual address, there's not much we can do, other than putting our own header on top of the user memory... but that's a lot of memory overhead...
bool virtual_is_remapped(void *p, OUT void **original_ptr) {
  pte_t *ppte;
  enum pt_level level = pt_walk(p, PGWALK_FULL, OUT &ppte);
  assert(FLAG_ISSET(*ppte, PG_V));

  // TODO: use instead one of the available PTE bits to note the fact that a page is dangless-remapped
  // currently this is only possible when we failed to allocate a dedicated virtual page for this allocation, and just falled back to proxying sysmalloc()
  if (level != PT_L1) {
    OUT *original_ptr = p;
    return false;
  }

  // since the original virtual address == physical address, we can just get the physical address and pretend it's a virtual address: specifically, it's the original virtual address, before we remapped it with virtual_remap()
  // this trickery allows us to get away with not maintaining mappings from the remapped virtual addresses to the original ones
  paddr_t pa = (paddr_t)(*ppte & PTE_FRAME_L1) + get_page_offset((uintptr_t)p, PT_L1);
  OUT *original_ptr = (void *)pa;
  return true;
}

void *dangless_malloc(size_t size) {
  void *p = sysmalloc(size);
  if (!p) {
    DPRINTF("bmk_memalloc() returned NULL!\n");
    return NULL;
  }

  return virtual_remap(p, size);
}

void *dangless_calloc(size_t num, size_t size) {
  void *p = syscalloc(num, size);
  if (!p) {
    DPRINTF("bmk_memcalloc() returned NULL!\n");
    return NULL;
  }

  return virtual_remap(p, size);
}

void *dangless_realloc(void *p, size_t new_size) {
  void *newp = sysrealloc(p, new_size);
  if (!newp)
    return NULL;

  // TODO
  UNREACHABLE("Unimplemented!\n");
}

int dangless_posix_memalign(void **pp, size_t align, size_t size) {
  int result = sysmemalign(pp, align, size);
  if (result != 0)
    return result;

  *pp = virtual_remap(*pp, size);
  return 0;
}

// Invalidates a virtual page that was remapped with virtual_remap(), by marking its PTE as dead.
// This allows us to detect a dangling pointer attempted access if we hook into the pagefault interrupt.
static void virtual_invalidate_pte(pte_t *ppte) {
  *ppte = DEAD_PTE;
}

static bool virtual_invalidate(void *p, size_t npages, OUT void **original_ptr) {
  if (!virtual_is_remapped(p, OUT original_ptr))
    return false;

  size_t page;
  for (page = 0; page < npages; page++) {
    // TODO: this could be optimised, since usually we'll be touching adjecent PTEs
    pte_t *ppte;
    enum pt_level level = pt_walk((uint8_t *)p + (page * PGSIZE), PGWALK_FULL, OUT &ppte);
    assert(level == PT_L1);
    assert(FLAG_ISSET(*ppte, PTE_V));

    virtual_invalidate_pte(ppte);
  }

  return true;
}

void dangless_free(void *p) {
  // compatibility with the libc free()
  if (!p)
    return;

  size_t npages = sysmalloc_usable_pages(p);

  void *original_ptr;
  virtual_invalidate(p, npages, OUT &original_ptr);

  sysfree(original_ptr);
}