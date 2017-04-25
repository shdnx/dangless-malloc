#include "mem.h"

// In rumprun, all physical memory is identity-mapped into virtual memory

void *paddr2vaddr(paddr_t pa) {
  return (void *)pa;
}

paddr_t vaddr2paddr(void *va) {
  return (paddr_t)va;
}