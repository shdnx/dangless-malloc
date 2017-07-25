#ifndef DANGLESS_CONFIG_H
#define DANGLESS_CONFIG_H

#ifndef DANGLESS_OVERRIDE_SYMBOLS
  #define DANGLESS_OVERRIDE_SYMBOLS 1
#endif

// Maximum number of free PML4 entries to auto-dedicate to dangless.
#ifndef DANGLESS_AUTO_DEDICATE_MAX_PML4ES
  #define DANGLESS_AUTO_DEDICATE_MAX_PML4ES 1uL
#endif

// Whether to use pthreads or just mock it.
#ifndef DANGLESS_USE_PTHREADS
  #define DANGLESS_USE_PTHREADS 0
#endif

// dangless_malloc.c
#define DGLMALLOC_DEBUG 1

// platform/virtual_remap.c
#define VIRTREMAP_DEBUG 1

// virtmem_alloc.c
#define VMALLOC_DEBUG 1
#define VMALLOC_STATS 1

#endif // DANGLESS_CONFIG_H
