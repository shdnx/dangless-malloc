#ifndef DANGLESS_MALLOC_H
#define DANGLESS_MALLOC_H

#define DO_ALIAS 0

#if defined(DO_ALIAS) && DO_ALIAS
  #define STRONG_ALIAS(NAME) __attribute__((alias(#NAME)))
#else
  #define STRONG_ALIAS(NAME) /* empty */
#endif

#include "common.h"

// Dedicate the given virtual memory region to be used by this allocator if possible.
int dangless_dedicate_vmem(void *start, void *end);

void *dangless_malloc(size_t sz) STRONG_ALIAS(malloc);
void *dangless_calloc(size_t num, size_t size) STRONG_ALIAS(calloc);
void *dangless_realloc(void *p, size_t new_size) STRONG_ALIAS(realloc);
int dangless_posix_memalign(void **pp, size_t align, size_t size) STRONG_ALIAS(posix_memalign);
void dangless_free(void *p) STRONG_ALIAS(free);

#endif // DANGLESS_MALLOC_H