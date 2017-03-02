#ifndef PHYSMEM_ALLOC_H
#define PHYSMEM_ALLOC_H

#include "common.h"
#include "mem.h"

// This is a memory allocator for 4K physical pages that are going to be in use for a very long time, and are very rarely or never freed (e.g. page tables).

// Allocates the specified number of 4K physical pages.
paddr_t pp_alloc(size_t npages);

// Allocates and zero-initializes the specified number of 4K physical pages.
paddr_t pp_zalloc(size_t npages);

// Frees a number of 4K physical pages starting at the given address.
// Note that it's not necessary for pp_free() call to free all pages allocated by an earlier pp_alloc() or pp_zalloc() call: partial freeing is also fine.
void pp_free(paddr_t pa, size_t npages);

inline static paddr_t pp_alloc_one(void) { return pp_alloc(1); }
inline static paddr_t pp_zalloc_one(void) { return pp_zalloc(1); }
inline static void pp_free_one(paddr_t pa) { return pp_free(pa, 1); }

#endif // PHYSMEM_ALLOC_H