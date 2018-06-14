#ifndef DUNETEST_H
#define DUNETEST_H

#include <string.h> // memset

#include "dangless/virtmem.h" // has to be included before libdune/dune.h, because we have some conflicting identifiers

#include "libdune/dune.h"

#include "dangless/common/types.h"
#include "dangless/dangless_malloc.h"

#include "ctestfx/ctestfx.h"

void dunetest_init(void);

// if the memregion is invalid, this will trigger a page fault, that will be handled by Dune's default pagefault handler
// TODO: more elaborate solution
#define EXPECT_VALID_MEMREGION(START, LEN) \
  memset((START), 0xEF, (LEN))

// TODO: more elegant solution
#define EXPECT_VALID_PTR(PTR) \
  *(u8 *)(PTR) = 0xBE;

#define EXPECT_INVALID_MEMREGION(START, LEN) \
  /* TODO */

#define EXPECT_INVALID_PTR(PTR) \
  EXPECT_EQUALS(pt_resolve_page((void *)(PTR), OUT_IGNORE), (paddr_t)0)

#define EXPECT_VALID_ALLOC(PTR, SIZE) \
  do { \
    EXPECT_NOT_NULL(PTR); \
    EXPECT_VALID_MEMREGION(PTR, SIZE); \
  } while (0)

#define TMALLOC(TYPE) \
  ({ \
    void *tmalloc_ptr = dangless_malloc(sizeof(TYPE)); \
    EXPECT_VALID_ALLOC(tmalloc_ptr, sizeof(TYPE)); \
    tmalloc_ptr; \
  })

#define TCALLOC(NUM, TYPE) \
 ({ \
    void *tcalloc_ptr = dangless_calloc((NUM), sizeof(TYPE)); \
    EXPECT_VALID_ALLOC(tcalloc_ptr, (NUM) * sizeof(TYPE)); \
    tcalloc_ptr; \
 })

#define TREALLOC(PTR, TYPE) \
 ({ \
    void *_ptr = dangless_realloc((PTR), sizeof(TYPE)); \
    EXPECT_VALID_ALLOC(_ptr, sizeof(TYPE)); \
    _ptr; \
 })

#define TFREE(PTR) dangless_free((PTR))

#define EXPECT_SAME_PAGE(PTR1, PTR2) \
 _ASSERT2(PTR1, PTR2, PG_IS_SAME, "expected address " #PTR1 " to be on the same 4K page as address " #PTR2)

#endif
