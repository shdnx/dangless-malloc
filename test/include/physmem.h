#ifndef PHYSMEM_H
#define PHYSMEM_H

#include "common.h"
#include "mem.h"

// This is a memory allocator for 4K physical pages.
// The assumption is that pages allocated via this allocator are rarely or never freed.

// Allocates the specified number of 4K physical pages.
paddr_t pp_alloc(size_t npages);

// Allocates and zero-initializes the specified number of 4K physical pages.
paddr_t pp_zalloc(size_t npages);

// Frees a number of 4K physical pages starting at the given address.
// Note that it's not necessary for pp_free() call to free all pages allocated by an earlier pp_alloc() or pp_zalloc() call: partial freeing is also fine.
void pp_free(paddr_t pa, size_t npages);

#endif // PHYSMEM_H