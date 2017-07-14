#ifndef VIRTMEM_ALLOC_H
#define VIRTMEM_ALLOC_H

#include "common.h"
#include "platform/mem.h"

// This is a virtual 4K page allocator.
// Initially, it doesn't own any memory, and any calls to vp_alloc() will fail. Assign memory to it by calling vp_free() or vp_free_region(): this is the memory that will be returned by subsequent vp_alloc() calls.
// In general, we cannot just use mmap() and munmap(), because e.g. on Rumprun, that'd also allocate physical memory pages.

void *vp_alloc(size_t npages);

// p: doesn't need to be a memory region previously allocated using vp_alloc().
// Even if it is, don't need to free the entire previously allocated region.
int vp_free(void *p, size_t npages);

inline static void *vp_alloc_one(void) { return vp_alloc(1); }
inline static int vp_free_one(void *p) { return vp_free(p, 1); }

inline static int vp_free_region(void *start, void *end) {
  return vp_free(start, ((vaddr_t)end - (vaddr_t)start) / PGSIZE);
}

#endif // VIRTMEM_ALLOC_H