#ifndef MEM_H
#define MEM_H

#include "common.h"

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

// x86-64 is assumed
#define PGSHIFT 12
#define PGSIZE (1uL << PGSHIFT)

// Given a physical address of a page table, gives the virtual address where that page table is mapped.
void *pt_paddr2vaddr(paddr_t pa);

#endif // MEM_H