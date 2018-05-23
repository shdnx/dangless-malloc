#ifndef MEM_H
#define MEM_H

#include "common.h"

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

// x86-64 is assumed
#define PGSHIFT 12
#define PGSIZE (1uL << PGSHIFT)

// Format string for pointers. This will cause a bunch of -Wformat warnings, but unfortunately, %016p is not valid.
//#define FMT_PTR "0x%016lx"
#define FMT_PTR "%p"

// Offsets a physical or virtual memory address by a given integer number of pages.
#define PG_OFFSET(BASE, NPAGES) \
  ((__typeof((BASE)))((uintptr_t)(BASE) + (NPAGES) * PGSIZE))

// Whether the two pointers/addresses are located on the same 4K page.
#define PG_IS_SAME(PTR1, PTR2) \
  ((uintptr_t)(PTR1) / PGSIZE == (uintptr_t)(PTR2) / PGSIZE)

// Given a physical address of a page table, gives the virtual address where that page table is mapped.
void *pt_paddr2vaddr(paddr_t pa);

#endif // MEM_H
