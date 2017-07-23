#ifndef MEM_H
#define MEM_H

#include "common.h"

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

// x86-64 is assumed
#define PGSHIFT 12
#define PGSIZE (1uL << PGSHIFT)

// Offsets a physical or virtual memory address by a given integer number of pages.
#define PG_OFFSET(BASE, NPAGES) \
  ((__typeof((BASE)))((uintptr_t)(BASE) + (NPAGES) * PGSIZE))

// Given a physical address of a page table, gives the virtual address where that page table is mapped.
void *pt_paddr2vaddr(paddr_t pa);

#endif // MEM_H
