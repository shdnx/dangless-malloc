#ifndef MEM_H
#define MEM_H

#include "common.h"

// TODO: there should be a definition for this in the kernel as well
typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

// TODO: this must be also defined somewhere in the kernel
#define PAGE_SIZE 4096

#endif // MEM_H