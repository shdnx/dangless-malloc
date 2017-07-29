#ifndef DUNETEST_H
#define DUNETEST_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libdune/dune.h"

#include "testfx.h"

void dunetest_init(void);

// if the memregion is invalid, this will trigger a page fault, that will be handled by Dune's default pagefault handler
// TODO: more elaborate solution
#define ASSERT_VALID_MEMREGION(START, LEN) \
  memset((START), 0xEF, (LEN))

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

#define TFREE(PTR) dangless_free((PTR))

#endif
