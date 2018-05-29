#ifndef DUNETEST_H
#define DUNETEST_H

#include <string.h> // memset

#include "libdune/dune.h"

#include "dangless/dangless_malloc.h"
#include "dangless/virtmem.h"

#include "testfx/testfx.h"

void dunetest_init(void);

// if the memregion is invalid, this will trigger a page fault, that will be handled by Dune's default pagefault handler
// TODO: more elaborate solution
#define ASSERT_VALID_MEMREGION(START, LEN) \
  memset((START), 0xEF, (LEN))

// TODO: more elegant solution
#define ASSERT_VALID_PTR(PTR) \
  *(uint8_t *)(PTR) = 0xBE;

#define ASSERT_INVALID_MEMREGION(START, LEN) \
  /* TODO */

#define ASSERT_INVALID_PTR(PTR) \
  ASSERT_EQUALS(pt_resolve_page((void *)(PTR), NULL), (paddr_t)0);

#define ASSERT_VALID_ALLOC(PTR, SIZE) \
  do { \
    ASSERT_NOT_NULL(PTR); \
    ASSERT_VALID_MEMREGION(PTR, SIZE); \
  } while (0)

#define TMALLOC(TYPE) \
  ({ \
    void *tmalloc_ptr = dangless_malloc(sizeof(TYPE)); \
    ASSERT_VALID_ALLOC(tmalloc_ptr, sizeof(TYPE)); \
    tmalloc_ptr; \
  })

#define TCALLOC(NUM, TYPE) \
 ({ \
    void *tcalloc_ptr = dangless_calloc((NUM), sizeof(TYPE)); \
    ASSERT_VALID_ALLOC(tcalloc_ptr, (NUM) * sizeof(TYPE)); \
    tcalloc_ptr; \
 })

#define TREALLOC(PTR, TYPE) \
 ({ \
    void *_ptr = dangless_realloc((PTR), sizeof(TYPE)); \
    ASSERT_VALID_ALLOC(_ptr, sizeof(TYPE)); \
    _ptr; \
 })

#define TFREE(PTR) dangless_free((PTR))

#define ASSERT_SAME_PAGE(PTR1, PTR2) \
 _ASSERT2(PTR1, PTR2, PG_IS_SAME, "expected address " #PTR1 " to be on the same 4K page as address " #PTR2)

#endif
