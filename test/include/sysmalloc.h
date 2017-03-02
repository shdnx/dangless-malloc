#ifndef SYSMALLOC_H
#define SYSMALLOC_H

#include <stddef.h>

// Original, non-overridden functions
void *sysmalloc(size_t sz);
void *syscalloc(size_t num, size_t size);
void *sysrealloc(void *p, size_t sz);
int sysmemalign(void **pp, size_t align, size_t sz);
void sysfree(void *p);

#define MALLOC(TYPE) ((TYPE *)sysmalloc(sizeof(TYPE)))
#define FREE(P) (sysfree((P)))

#endif // SYSMALLOC_H