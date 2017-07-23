#ifndef VIRTMEM_H
#define VIRTMEM_H

#include "common.h"

#include "platform/mem.h"

enum {
  RING_KERNEL = 0x0,
  RING_USER = 0x3,
  RING_MASK = 0x3
};

// Determines whether we're currently in kernel (ring 0) mode.
static inline bool in_kernel_mode(void) {
  u16 cs;
  asm("mov %%cs, %0" : "=r" (cs));
  return (cs & RING_MASK) == RING_KERNEL;
}

// we treat entries on all page levels the same way and call them PTEs for simplicity
typedef u64 pte_t;

#define PT_BITS_PER_LEVEL 9
#define PT_NUM_ENTRIES (1uL << PT_BITS_PER_LEVEL)

enum pte_flags {
  PTE_V = 0x1, // valid
  PTE_W = 0x2, // writable
  PTE_U = 0x4, // user accessible
  PTE_PS = 0x80, // page size (everywhere but in PTL1)
  PTE_PAT = 0x80, // PAT (on PTE)
  PTE_NX = 0x8000000000000000uL // non-executable
};

enum {
  PTE_FRAME = 0x000ffffffffff000uL,
  PTE_FRAME_4K = PTE_FRAME,
  PTE_FRAME_L1 = PTE_FRAME_4K,

  PTE_FRAME_2M = 0x000fffffffe00000uL,
  PTE_FRAME_L2 = PTE_FRAME_2M,

  PTE_FRAME_1G = 0x000fffffc0000000uL,
  PTE_FRAME_L3 = PTE_FRAME_1G
};

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

// Calculates the number of 4K pages mapped by an entry at the given pagetable level.
static inline size_t pt_num_mapped_pages(enum pt_level level) {
  return 1uL << ((level - 1) * PT_BITS_PER_LEVEL);
}

static inline unsigned pt_level_shift(enum pt_level level) {
  return PGSHIFT + (level - 1) * PT_BITS_PER_LEVEL;
}

static inline size_t pt_level_offset(vaddr_t va, enum pt_level level) {
  return (va >> pt_level_shift(level)) & (PT_NUM_ENTRIES - 1);
}

// Gets the offset of the specified address into a page mapped by the specified pagetable level.
static inline size_t get_page_offset(uintptr_t addr, enum pt_level pt_level) {
  uintptr_t page_offset_mask = (1uL << pt_level_shift(pt_level)) - 1;
  return (size_t)(addr & page_offset_mask);
}

// Reads the value of control register 3, i.e. the root of the page table hierarchy (the physical address of the currently active PML4).
static inline u64 rcr3(void) {
  u64 cr3;
  asm("movq %%cr3, %0" : "=r" (cr3));
  return cr3;
}

enum {
  PGWALK_FULL = PT_L1
};

// Performs a pagetable walk on the specified virtual address going until the specified level at most.
// The page walk may terminate early, in case of a present bit missing on a higher-level PTE.
// result_pte is filled out with a pointer to the last PTE checked, and the number of the last level returned.
enum pt_level pt_walk(void *p, enum pt_level requested_level, OUT pte_t **result_ppte);

// Gets the physical address of the page backing the given virtual address.
paddr_t pt_resolve_page(void *p, OUT enum pt_level *page_level);

// Gets the physical address corresponding to the given virtual address.
paddr_t pt_resolve(void *p);

int pt_map_page(paddr_t pa, vaddr_t va, pte_t flags);
int pt_map_region(paddr_t pa, vaddr_t va, size_t size, pte_t flags);

// Unmap the given virtual page from the current address space at the specified level.
int pt_unmap_page(vaddr_t va, enum pt_level on_level);

// Blatantly stolen from ix-dune/libdune/dune.h, except renamed
static inline void tlb_flush_one(void *addr) {
  asm ("invlpg (%0)" :: "r" (addr) : "memory");
}

static inline void tlb_flush_all(void) {
  asm ("mov %%cr3, %%rax\n"
       "mov %%rax, %%cr3\n" ::: "rax");
}

#endif // VIRTMEM_H
