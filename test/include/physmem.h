#ifndef PHYSMEM_H
#define PHYSMEM_H

#include "common.h"
#include "mem.h"

// Allocates/frees an identity-mapped 4K physical memory page.
void *page_alloc(void);
void page_free(void *p);

#endif // PHYSMEM_H