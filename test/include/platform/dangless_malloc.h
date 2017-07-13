#ifndef DANGLESS_MALLOC_H
#define DANGLESS_MALLOC_H

#include "common.h"

// Dedicate the given virtual memory region to be used by this allocator if possible.
int dangless_dedicate_vmem(void *start, void *end);

void *dangless_malloc(size_t sz);
void *dangless_calloc(size_t num, size_t size);
void *dangless_realloc(void *p, size_t new_size);
int dangless_posix_memalign(void **pp, size_t align, size_t size);
void dangless_free(void *p);

#endif // DANGLESS_MALLOC_H