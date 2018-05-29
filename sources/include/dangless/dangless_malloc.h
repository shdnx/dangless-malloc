#ifndef DANGLESS_MALLOC_H
#define DANGLESS_MALLOC_H

#include <stdint.h>
#include <stdbool.h>

// Dedicate the given virtual memory region to be used by this allocator if possible.
int dangless_dedicate_vmem(void *start, void *end);

void *dangless_malloc(size_t sz) __attribute__((malloc));
void *dangless_calloc(size_t num, size_t size) __attribute__((malloc));
void *dangless_realloc(void *p, size_t new_size);
int dangless_posix_memalign(void **pp, size_t align, size_t size);
void dangless_free(void *p);

// Whether a dangless hook is currently being processed on the calling thread.
bool dangless_is_hook_running(void);

// Gets the original (canonical) pointer as returned by the system allocator of a dangless-remapped pointer.
void *dangless_get_canonical(void *p);

// Equivalent of malloc_usable_size() GNU extension.
// TODO: this is a GNU extension, should it be supported? E.g. Rumprun's implementation doesn't provide malloc_usable_size()
//size_t dangless_usable_size(void *p);

#endif
