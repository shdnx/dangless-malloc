#include <assert.h>

#include "platform/mem.h"

#include "dune.h"

void *pt_paddr2vaddr(paddr_t pa) {
  // The lowest physical address that stack uses.
  static paddr_t dune_stack_pa_base = 0;

  // The lowest physical address that mmap() yields.
  static paddr_t dune_mmap_pa_base = 0;

  if (!dune_stack_pa_base) {
    dune_stack_pa_base = dune_va_to_pa((void *)stack_base); // == phys_limit - GPA_STACK_SIZE = 511 GB

    dune_mmap_pa_base = dune_va_to_pa((void *)mmap_base); // == phys_limit - GPA_STACK_SIZE - GPA_MAP_SIZE = 512 GB - 1 GB - 63 GB = 448 GB
  }

  // The stack is highest: any page-tables have to be underneath it
  assert(pa < dune_stack_pa_base);

  if (pa >= dune_mmap_pa_base) {
    // this memory was allocated with mmap(): reverse dune_mmap_addr_to_pa()
    // PA = VA - mmap_base + phys_limit - GPA_STACK_SIZE - GPA_MAP_SIZE
    return (void *)(pa + mmap_base - phys_limit + GPA_STACK_SIZE + GPA_MAP_SIZE);
  } else {
    // this memory was not allocated via mmap(); assume identity-mapping
    return (void *)pa;
  }
}