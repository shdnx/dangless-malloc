#include <assert.h>

#include "dangless_malloc.h"
#include "virtmem.h"
#include "virtmem_alloc.h"

#include "platform/sysmalloc.h"
#include "platform/virtual_remap.h"

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

void *dangless_malloc(size_t size) {
  void *p = sysmalloc(size);
  if (!p)
    return NULL;

  int result;
  void *remapped_ptr;

  if ((result = vremap_map(p, size, OUT &remapped_ptr)) < 0) {
    DPRINTF("failed to remap sysmalloc's %p (size %zu); falling back to proxying (result %d)\n", p, size, result);
    return p;
  }

  return remapped_ptr;
}

void *dangless_calloc(size_t num, size_t size) {
  void *p = syscalloc(num, size);
  if (!p)
    return NULL;

  int result;
  void *remapped_ptr;

  if ((result = vremap_map(p, size, OUT &remapped_ptr)) < 0) {
    DPRINTF("failed to remap syscalloc's %p (size %zu); falling back to proxying (result %d)\n", p, size, result);
    return p;
  }

  return remapped_ptr;
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

  if ((result = vremap_map(*pp, size, OUT pp)) < 0) {
    DPRINTF("failed to remap sysmemalign's %p (size %zu); falling back to proxying (result %d)\n", *pp, size, result);
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

  void *original_ptr = p;
  int result = vremap_resolve(p, OUT &original_ptr);

  if (result == 0) {
    size_t npages = sysmalloc_usable_pages(original_ptr);
    DPRINTF("invalidating %zu virtual pages starting at %p...\n", npages, p);
    virtual_invalidate(p, npages);
  } else if (result < 0) {
    DPRINTF("failed to determine whether %p was remapped: assume not (result %d)\n", p, result);
  } else {
    DPRINTF("vremap_resolve returned %d, assume no remapping\n", result);
  }

  sysfree(original_ptr);
}