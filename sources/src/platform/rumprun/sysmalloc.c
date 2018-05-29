#include "dangless/common.h"
#include "dangless/platform/sysmalloc.h"

#include "rumprun.h"

#if SYSMALLOC_DEBUG
  #define LOG(...) vdprintf(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

// bmk-core/memalloc.h
enum {
  BMK_MEMALLOC_WHO = 2, // = BMK_MEMWHO_USER in bmk-core/memalloc.h
  BMK_ENOMEM = 12, // = BMK_ENOMEM in bmk-core/errno.h
  BMK_MEMALLOC_ALIGN = 16
};

RUMPRUN_DEF_FUNC(0xc440, void *, bmk_memalloc, unsigned long /*nbytes*/, unsigned long /*align*/, int /*enum bmk_memwho who*/);
RUMPRUN_DEF_FUNC(0xc650, void *, bmk_memcalloc, unsigned long /*num*/, unsigned long /*size*/, int /*enum bmk_memwho who*/);
RUMPRUN_DEF_FUNC(0xc750, void *, bmk_memrealloc_user, void */*cp*/, size_t /*nbytes*/);
RUMPRUN_DEF_FUNC(0xc6b0, void, bmk_memfree, void */*p*/, int /*enum bmk_memwho who*/);

void *sysmalloc(size_t sz) {
  return RUMPRUN_FUNC(bmk_memalloc)(sz, BMK_MEMALLOC_ALIGN, BMK_MEMALLOC_WHO);
}

void *syscalloc(size_t num, size_t size) {
  return RUMPRUN_FUNC(bmk_memcalloc)(num, size, BMK_MEMALLOC_WHO);
}

void *sysrealloc(void *p, size_t sz) {
  return RUMPRUN_FUNC(bmk_memrealloc_user)(p, sz);
}

int sysmemalign(void **pp, size_t align, size_t sz) {
  // unfortunately, rumprun doesn't directly expose a symbol to their posix_memalign() implementation, so I just blatantly stole it from librumprun_base/malloc.c
  void *p;
  int error = BMK_ENOMEM;

  if ((p = RUMPRUN_FUNC(bmk_memalloc)(sz, align, BMK_MEMALLOC_WHO)) != NULL) {
    error = 0;
    *pp = p;
  }

  return error;
}

void sysfree(void *p) {
  RUMPRUN_FUNC(bmk_memfree)(p, BMK_MEMALLOC_WHO);
}

// TODO: this is very bad, very tight coupling with the internal implementation details of BMK's memalloc.c - however, the only alternative is adding our own header, with its own overhead...
// unfortunately rumprun doesn't support the malloc_usable_size() GNU extension
size_t sysmalloc_usable_pages(void *p) {
/*
// bmk-core/memalloc.c:
struct memalloc_hdr {
  uint32_t  mh_alignpad; // padding for alignment
  uint16_t  mh_magic;    // magic number
  uint8_t   mh_index;    // bucket number
  uint8_t   mh_who;      // who allocated
};

// ...

#define MINSHIFT 5
#define LOCALBUCKETS (BMK_PCPU_PAGE_SHIFT - MINSHIFT)

// ...

  if (bucket >= LOCALBUCKETS) {
    hdr = bmk_pgalloc(bucket+MINSHIFT - BMK_PCPU_PAGE_SHIFT);
  } else {
    hdr = bucketalloc(bucket);
  }

// bmk-core/pgalloc.c:
#define order2size(_order_) (1UL<<(_order_ + BMK_PCPU_PAGE_SHIFT))
 */

#define BMK_MEMALLOC_LOCALBUCKETS 7

  // read the mh_index field from the memalloc_hdr that's placed before the user memory
  uint8_t bucket_index = *((uint8_t *)p - 2);
  if (bucket_index <= BMK_MEMALLOC_LOCALBUCKETS)
    return 1;

  int pgalloc_order = bucket_index - BMK_MEMALLOC_LOCALBUCKETS;
  return (size_t)(1uL << pgalloc_order);
}
