#include "sysmalloc.h"
#include "common.h"
#include "rumprun.h"

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