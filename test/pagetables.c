#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <bmk-core/pgalloc.h> // bmk_pgalloc_one()

#include "common.h"
#include "pagetables.h"

uint64_t rcr3(void) {
  uint64_t val;
  asm("movq %%cr3, %0" : "=r" (val));
  return val;
}

#define PT_LEVEL_OFFSET(VA, LEVEL) (((VA) & CONCAT3(L, LEVEL, _MASK)) >> CONCAT3(L, LEVEL, _SHIFT))

void pte_dump(pte_t pte, unsigned level) {
  printf("PTE addr = 0x%lx", pte & PG_FRAME);

#define HANDLE_BIT(BIT) if (pte & (BIT)) printf(" " #BIT)

  HANDLE_BIT(PG_V);
  HANDLE_BIT(PG_RW);
  HANDLE_BIT(PG_u);
  HANDLE_BIT(PG_NX);

  if (level == 1)
    HANDLE_BIT(PG_PAT);
  else
    HANDLE_BIT(PG_PS);

#undef HANDLE_BIT

  printf(" (raw: 0x%lx)\n", pte);
}

unsigned pt_walk(void *p, unsigned requested_level, OUT pte_t **result_ppte) {
  vaddr_t va = (vaddr_t)p;
  paddr_t paddr = (paddr_t)rcr3();
  pte_t *ppte;

#define WALK_LEVEL(LVL) \
    ppte = &((pte_t *)paddr)[PT_LEVEL_OFFSET(va, LVL)]; \
    if (!FLAG_ISSET(*ppte, PG_V) \
        || (LVL != 1 && FLAG_ISSET(*ppte, PG_PS)) \
        || requested_level == LVL) { \
      OUT *result_ppte = ppte; \
      return LVL; \
    } \
    paddr = (paddr_t)(*ppte & PG_FRAME)

  WALK_LEVEL(4);
  WALK_LEVEL(3);
  WALK_LEVEL(2);
  WALK_LEVEL(1);

#undef WALK_LEVEL

  OUT *result_ppte = ppte;
  return 1;
}

paddr_t get_paddr_page(void *p, OUT unsigned *page_level) {
  pte_t *ppte;
  unsigned level = pt_walk(p, PGWALK_FULL, OUT &ppte);

  if (!FLAG_ISSET(*ppte, PG_V))
    return 0;

  if (page_level)
    OUT *page_level = level;

  paddr_t pa;
  switch (level) {
  case 1:
    return (*ppte & PG_FRAME);

  case 2:
    assert(FLAG_ISSET(*ppte, PG_PS));
    return (*ppte & PG_2MFRAME);

  case 3:
    assert(FLAG_ISSET(*ppte, PG_PS));
    return (*ppte & PG_1GFRAME);
  }

  __builtin_unreachable();
}

size_t get_page_offset(uintptr_t addr, unsigned pt_level) {
  uintptr_t page_offset_mask = (1 << (12 + (pt_level - 1) * 9)) - 1;
  return (size_t)(addr & page_offset_mask);
}

paddr_t get_paddr(void *p) {
  unsigned level;
  paddr_t pa = get_paddr_page(p, OUT &level);
  if (!pa)
    return 0;

  return pa + get_page_offset((uintptr_t)p, level);
}

int pt_map(paddr_t pa, vaddr_t va, pte_t flags) {
  assert(pa % PAGE_SIZE == 0);

  pte_t *ppte;
  unsigned level = pt_walk((void *)va, PGWALK_FULL, OUT &ppte);
  pte_t *pt;

  #define CREATE_LEVEL(LVL) \
    pt = (pte_t *)bmk_pgalloc_one(); \
    assert((uintptr_t)pt % PAGE_SIZE == 0); \
    \
    if (!pt) { \
      /* TODO: clean-up */ \
      return -1; \
    } \
    \
    *ppte = (uintptr_t)pt | flags | PG_V; \
    ppte = &pt[PT_LEVEL_OFFSET(va, EVAL(LVL))]
  
  switch (level) {
  case 4: CREATE_LEVEL(3);
  case 3: CREATE_LEVEL(2);
  case 2: CREATE_LEVEL(1);
  }

  *ppte = pa | flags | PG_V;
  return 0;
}

int pt_unmap(vaddr_t va) {
  pte_t *ppte;
  unsigned level = pt_walk((void *)va, PGWALK_FULL, OUT &ppte);
  if (level != 1)
    return -1;

  *ppte = 0;
  return 0;
}