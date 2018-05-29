#ifndef DANGLESS_CONFIG_H
#define DANGLESS_CONFIG_H

#include "dangless/buildconfig.h"

// TODO: turn all of these into build config things?

// dangless_malloc.c
#define DGLMALLOC_DEBUG 1

// platform/virtual_remap.c
#define VIRTREMAP_DEBUG 1

// platform/sysmalloc.c
//#define SYSMALLOC_DEBUG 1

// platform/init.c
#define INIT_DEBUG 1

// virtmem_alloc.c
#define VMALLOC_DEBUG 1
#define VMALLOC_STATS 1

#endif // DANGLESS_CONFIG_H
