#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "rumpkern.h"
#include "physmem.h"
#include "virtmem.h"

#define PAGE_ALLOC_VERIFY 1

// rumprun/include/bmk-core/pgalloc.h
RUMPKERN_DECL_FUNC(0xce60, void *, bmk_pgalloc, int /*order*/);
RUMPKERN_DECL_FUNC(0xce70, void, bmk_pgfree, void * /*ptr*/, int /*order*/);

static void *page_alloc_do(void) {
  // unfortunately, there's no way to use malloc(), not even if we mangle the first 0x10 bytes (the malloc header), since the virtual address it gives back is backed by a 2M hugepage... :(
  // we cannot simply call bmk_pgalloc_one(), because basically we'd have to link against the entire kernel, use their ldscript, and so on... probably not possible
  // dlopen(), etc. is not supported
  // in rumprun.o, bmk_pgalloc() is shown to be at 0xce60, and scanning the memory for its signature found it at 0x10ee60, so we can just call directly there...
  return RUMPKERN_FUNC(bmk_pgalloc)(0);
}

void page_free(void *p) {
  RUMPKERN_FUNC(bmk_pgfree)(p, 0);
}

void *page_alloc(void) {
  void *p = page_alloc_do();
  assert((vaddr_t)p % PAGE_SIZE == 0);

#if PAGE_ALLOC_VERIFY
  if (!p)
    return NULL;

  // verify that there this virtual address is indeed mapped to a 4K physical page
  enum pt_level level;
  paddr_t pa = get_paddr_page(p, OUT &level);

  printf("Allocated virtual page at %p, phys page addr = 0x%lx, level = %u\n", p, pa, level);

  if (!pa || level != PT_4K) {
    page_free(p);
    return NULL;
  }

#endif

  return p;
}