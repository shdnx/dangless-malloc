#include "sysmalloc.h"
#include "common.h"
#include "rumprun.h"

// bmk-core/memalloc.h
enum {
  BMK_MEMALLOC_WHO = 2 // = BMK_MEMWHO_USER
};

RUMPRUN_DEF_FUNC(0xc440, void *, bmk_memalloc, unsigned long /*nbytes*/, unsigned long /*align*/, int /* enum bmk_memwho who */);
RUMPRUN_DEF_FUNC(0xc6b0, void, bmk_memfree, void * /* p */, int /* enum bmk_memwho who */);

enum {
  BMK_MEMALLOC_ALIGN = 16 // just to be safe
};

void *sysmalloc(size_t sz) {
  return RUMPRUN_FUNC(bmk_memalloc)(sz, BMK_MEMALLOC_ALIGN, BMK_MEMALLOC_WHO);
}

void sysfree(void *p) {
  RUMPRUN_FUNC(bmk_memfree)(p, BMK_MEMALLOC_WHO);
}