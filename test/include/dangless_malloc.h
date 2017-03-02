#ifndef DANGLESS_MALLOC_H
#define DANGLESS_MALLOC_H

#include "common.h"

// TODO: override the malloc/free symbols
// TODO: calloc, realloc, memalign, etc.

// Dedicate the given virtual memory region to be used by this allocator if possible.
int dangless_dedicate_vmem(void *start, void *end);

void *dangless_malloc(size_t sz);
void dangless_free(void *p);

#endif // DANGLESS_MALLOC_H