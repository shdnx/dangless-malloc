#ifndef SYSMALLOC_H
#define SYSMALLOC_H

#include <stddef.h>

// Original, non-overridden functions
void *sysmalloc(size_t sz);
void sysfree(void *p);

#define MALLOC(TYPE) ((TYPE *)sysmalloc(sizeof(TYPE)))
#define FREE(P) (sysfree((P)))

#endif // SYSMALLOC_H