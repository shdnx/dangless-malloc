#ifndef DANGLESS_PLATFORM_SYSMALLOC_H
#define DANGLESS_PLATFORM_SYSMALLOC_H

#include <stddef.h>

// Original, non-overridden functions
void *sysmalloc(size_t sz);
void *syscalloc(size_t num, size_t size);
void *sysrealloc(void *p, size_t sz);
int sysmemalign(void **pp, size_t align, size_t sz);
void sysfree(void *p);

// For memory allocated by sysmalloc(), returns the number of usable bytes in the allocation.
size_t sysmalloc_usable_size(void *p);

// For memory allocated by sysmalloc(), returns the number of 4K pages it spans (minimum 1).
size_t sysmalloc_usable_pages(void *p);

#define MALLOC(TYPE) ((TYPE *)sysmalloc(sizeof(TYPE)))
#define FREE(P) (sysfree((P)))

#endif
