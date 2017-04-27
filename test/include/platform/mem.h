#ifndef MEM_H
#define MEM_H

#include "common.h"

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

// x86-64 is assumed
#define PGSHIFT 12
#define PGSIZE (1uL << PGSHIFT)

// If a virtual-to-physical identity mapping exists, these functions translate between the virtual and physical addresses.
// TODO: what if not? Can that even happen?
void *paddr2vaddr(paddr_t pa);
paddr_t vaddr2paddr(void *va);

// For Dune: there's dune_va_to_pa() in dune.h, and also mmap_base and stack_base

#endif // MEM_H