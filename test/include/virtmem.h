#ifndef VIRTMEM_H
#define VIRTMEM_H

#include "common.h"
#include "mem.h"

#include <arch/amd64/include/pte.h>

// TODO: pt_entry_t and pd_entry_t
typedef uint64_t pte_t;

// Reads the value of control register 3.
uint64_t rcr3(void);

enum pt_level {
  PT_INVALID = 0,

  PT_L1 = 1,
  PT_4K = PT_L1,

  PT_L2 = 2,
  PT_2M = PT_L2,

  PT_L3 = 3,
  PT_1G = PT_L3,

  PT_L4 = 4,
  PT_512G = PT_L4
};

void pte_dump(pte_t pte, enum pt_level level);

enum {
  PGWALK_FULL = PT_L1
};

// Performs a pagetable walk on the specified virtual address going until the specified level at most.
// The page walk may terminate early, in case of a present bit missing on a higher-level PTE.
// result_pte is filled out with a pointer to the last PTE checked, and the number of the last level returned.
enum pt_level pt_walk(void *p, enum pt_level requested_level, OUT pte_t **result_ppte);

// Gets the offset of the specified address into a page mapped by the specified pagetable level.
size_t get_page_offset(uintptr_t addr, enum pt_level pt_level);

// Gets the physical address of the page backing the given virtual address.
paddr_t get_paddr_page(void *p, OUT enum pt_level *page_level);

// Gets the physical address corresponding to the given virtual address.
paddr_t get_paddr(void *p);

int pt_map(paddr_t pa, vaddr_t va, pte_t flags);
int pt_unmap(vaddr_t va);

#endif // VIRTMEM_H