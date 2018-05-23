#include "platform/mem.h"

// In rumprun, all physical memory is identity-mapped into virtual memory.

void *pt_paddr2vaddr(paddr_t pa) {
  return (void *)pa;
}