#ifndef MEM_H
#define MEM_H

#include "common.h"

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

#define PGSHIFT 12
#define PGSIZE (1uL << PGSHIFT)

// All physical memory is identity-mapped into virtual memory
#define paddr2vaddr(PA) ((void *)(PA))

#endif // MEM_H