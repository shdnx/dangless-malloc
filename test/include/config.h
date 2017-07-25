#ifndef DANGLESS_CONFIG_H
#define DANGLESS_CONFIG_H

#define DANGLESS_OVERRIDE_SYMBOLS 1

// Maximum number of free PML4 entries to auto-dedicate to dangless.
#define DANGLESS_AUTO_DEDICATE_MAX_PML4ES 1

// dangless_malloc.c
#define DGLMALLOC_DEBUG 1

// platform/virtual_remap.c
#define VIRTREMAP_DEBUG 1

// virtmem_alloc.c
#define VMALLOC_DEBUG 1
#define VMALLOC_STATS 1

#endif // DANGLESS_CONFIG_H
