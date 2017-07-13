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

// TODO: this only supports <= PAGESIZE allocation
static bool virtual_remap(void *p, size_t size, OUT void **remapped_ptr) {
  assert(p);

  void *va = vp_alloc_one();
  if (!va) {
    DPRINTF("could not allocate virtual memory page to remap to\n");
    return false;
  }

#if DGLMALLOC_DEBUG
  {
    enum pt_level level;
    paddr_t pa = pt_resolve_page(va, OUT &level);
    assert(!pa && "Allocated virtual page is already mapped to a physical address!");
  }
#endif

  // this assumes that an identity mapping exists between the physical and virtual memory
  paddr_t pa = (paddr_t)p;
  paddr_t pa_page = ROUND_DOWN(pa, PGSIZE);

  // this assumes that the allocated virtual memory region is backed by a contiguous physical memory region
  int result;
  if ((result = pt_map_region(pa_page, (vaddr_t)va, ROUND_UP(size, PGSIZE), PTE_W | PTE_NX)) < 0) {
    DPRINTF("could not remap pa 0x%lx to va %p, code %d\n", pa_page, va, result);

    // try to give back the virtual memory page - this may fail, but we can't do anything about it
    vp_free_one(va);
    return false;
  }

  OUT *remapped_ptr = (uint8_t *)va + get_page_offset((uintptr_t)p, PT_4K);
  return true;
}

// TODO: this function is very rumprun-specific
// how could we make this platform-independent? when physical address != original virtual address, there's not much we can do, other than putting our own header on top of the user memory... but that's a lot of memory overhead...
bool virtual_is_remapped(void *p, OUT void **original_ptr) {
  pte_t *ppte;
  enum pt_level level = pt_walk(p, PGWALK_FULL, OUT &ppte);
  assert(FLAG_ISSET(*ppte, PG_V));

  // TODO: use instead one of the available PTE bits to note the fact that a page is dangless-remapped (unless somebody already uses those bits?)
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
  if (!p)
    return NULL;

  void *remapped_ptr;
  if (virtual_remap(p, size, OUT &remapped_ptr)) {
    return remapped_ptr;
  } else {
    DPRINTF("failed to remap sysmalloc's %p (size %zu); falling back to proxying\n", p, size);
    return p;
  }
}

void *dangless_calloc(size_t num, size_t size) {
  void *p = syscalloc(num, size);
  if (!p)
    return NULL;

  void *remapped_ptr;
  if (virtual_remap(p, size, OUT &remapped_ptr)) {
    return remapped_ptr;
  } else {
    DPRINTF("failed to remap syscalloc's %p (size %zu); falling back to proxying\n", p, size);
    return p;
  }
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

  if (!virtual_remap(*pp, size, OUT pp)) {
    DPRINTF("failed to remap sysmemalign's %p (size %zu); falling back to proxying\n", *pp, size);
  }

  return 0;
}

// Invalidates a virtual page that was remapped with virtual_remap(), by marking its PTE as dead.
// This allows us to detect a dangling pointer attempted access if we hook into the pagefault interrupt.
static void virtual_invalidate_pte(pte_t *ppte) {
  *ppte = DEAD_PTE;
}

static void virtual_invalidate(void *p, size_t npages) {
  size_t page = 0;
  while (page < npages) {
    pte_t *ppte;
    enum pt_level level = pt_walk((uint8_t *)p + page * PGSIZE, PGWALK_FULL, OUT &ppte);
    assert(level == PT_L1);
    assert(FLAG_ISSET(*ppte, PTE_V));

    // optimised for invalidating adjecent PTEs in a PT
    size_t pte_base_offset = pt_level_offset((vaddr_t)p, PT_L1);
    size_t nptes = MIN(PT_NUM_ENTRIES - pte_base_offset, npages - page);

    size_t pte_offset;
    for (pte_offset = 0; pte_offset < nptes; pte_offset++) {
      virtual_invalidate_pte(&ppte[pte_offset]);
    }

    page += nptes;
  }
}

void dangless_free(void *p) {
  // compatibility with the libc free()
  if (!p)
    return;

  size_t npages = sysmalloc_usable_pages(p);

  void *original_ptr;
  if (virtual_is_remapped(p, OUT &original_ptr)) {
    virtual_invalidate(p, npages);
  }

  sysfree(original_ptr);
}