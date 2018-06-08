#ifndef DANGLESS_PLATFORM_MEM_H
#define DANGLESS_PLATFORM_MEM_H

#include "dangless/common.h"

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

// x86-64 is assumed
#define PGSHIFT 12
#define PGSIZE (1uL << PGSHIFT)

// Offsets a physical or virtual memory address by a given integer number of pages.
#define PG_OFFSET(BASE, NPAGES) \
  ((typeof((BASE)))((uintptr_t)(BASE) + (NPAGES) * PGSIZE))

// Whether the two pointers/addresses are located on the same 4K page.
#define PG_IS_SAME(PTR1, PTR2) \
  ((uintptr_t)(PTR1) / PGSIZE == (uintptr_t)(PTR2) / PGSIZE)

// Given a physical address of a page table, gives the virtual address where that page table is mapped.
void *pt_paddr2vaddr(paddr_t pa);

#define NUM_SPANNED_PAGES(ADDR, SIZE) ( \
    (size_t)( \
      ROUND_UP((uintptr_t)(ADDR) + (SIZE), PGSIZE) \
      - ROUND_DOWN((uintptr_t)(ADDR), PGSIZE) \
    ) \
    / PGSIZE \
  )

#endif
