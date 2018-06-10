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

// Gets the page index that the specified pointer or memory address falls into.
#define PG_INDEX(PTR) (size_t)((uintptr_t)(PTR) / PGSIZE)

// Whether the two pointers/addresses are located on the same 4K page.
#define PG_IS_SAME(PTR1, PTR2) (PG_INDEX(PTR1) == PG_INDEX(PTR2))

// Given a physical address of a page table, gives the virtual address where that page table is mapped.
void *pt_paddr2vaddr(paddr_t pa);

// Calculates the number of pages spanned by the address range [ADDR, ADDR + SIZE). Does not access any memory.
#define NUM_SPANNED_PAGES(ADDR, SIZE) ( \
    (size_t)( \
      ROUND_UP((uintptr_t)(ADDR) + (SIZE), PGSIZE) \
      - ROUND_DOWN((uintptr_t)(ADDR), PGSIZE) \
    ) \
    / PGSIZE \
  )

#endif
