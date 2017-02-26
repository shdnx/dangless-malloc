#ifndef PAGETABLES_H
#define PAGETABLES_H

#include "common.h"

#include <arch/amd64/include/pte.h>

// TODO: there should be a definition for this in the kernel as well
typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

// TODO: pt_entry_t and pd_entry_t
typedef uint64_t pte_t;

// TODO: this must be also defined somewhere in the kernel
#define PAGE_SIZE 4096

// Reads the value of control register 3.
uint64_t rcr3(void);

void pte_dump(pte_t pte, unsigned level);

enum {
  PGWALK_PT = 1,
  PGWALK_PTL1 = PGWALK_PT,
  PGWALK_FULL = PGWALK_PT,

  PGWALK_PD = 2,
  PGWALK_PTL2 = PGWALK_PD,

  PGWALK_PTPD = 3,
  PGWALK_PTL3 = PGWALK_PTPD,

  PGWALK_PML4 = 4,
  PGWALK_PTL4 = PGWALK_PML4
};

enum {
  PT_L1 = 1,
  PT_4K = PT_L1,

  PT_L2 = 2,
  PT_2M = PT_L2,

  PT_L3 = 3,
  PT_1G = PT_L3,

  PT_L4 = 4,
  PT_512G = PT_L4
};

// Performs a pagetable walk on the specified virtual address going until the specified level at most.
// The page walk may terminate early, in case of a present bit missing on a higher-level PTE.
// result_pte is filled out with a pointer to the last PTE checked, and the number of the last level returned.
unsigned pt_walk(void *p, unsigned requested_level, OUT pte_t **result_ppte);

// Gets the offset of the specified address into a page mapped by the specified pagetable level.
size_t get_page_offset(uintptr_t addr, unsigned pt_level);

// Gets the physical address of the page backing the given virtual address.
paddr_t get_paddr_page(void *p, OUT unsigned *page_level);

// Gets the physical address corresponding to the given virtual address.
paddr_t get_paddr(void *p);

int pt_map(paddr_t pa, vaddr_t va, pte_t flags);
int pt_unmap(vaddr_t va);

#endif // PAGETABLES_H