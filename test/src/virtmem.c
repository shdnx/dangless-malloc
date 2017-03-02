#include <stdio.h>
#include <assert.h>

#include "common.h"
#include "virtmem.h"
#include "physmem_alloc.h"

#if VIRTMEM_DEBUG
  #define VIRTMEM_DPRINTF(...) dprintf("[virtmem] " __VA_ARGS__)
#else
  #define VIRTMEM_DPRINTF(...) /* empty */
#endif

uint64_t rcr3(void) {
  uint64_t val;
  asm("movq %%cr3, %0" : "=r" (val));
  return val;
}

#define PT_LEVEL_OFFSET(VA, LEVEL) (((VA) & CONCAT3(L, LEVEL, _MASK)) >> CONCAT3(L, LEVEL, _SHIFT))

void pte_dump(FILE *stream, pte_t pte, enum pt_level level) {
  fprintf(stream, "PTE addr = 0x%lx", pte & PG_FRAME);

#define HANDLE_BIT(BIT) if (pte & (BIT)) fprintf(stream, " " #BIT)

  HANDLE_BIT(PG_V);
  HANDLE_BIT(PG_RW);
  HANDLE_BIT(PG_u);
  HANDLE_BIT(PG_NX);

  if (level == 1)
    HANDLE_BIT(PG_PAT);
  else
    HANDLE_BIT(PG_PS);

#undef HANDLE_BIT

  fprintf(stream, " (raw: 0x%lx)\n", pte);
}

enum pt_level pt_walk(void *p, enum pt_level requested_level, OUT pte_t **result_ppte) {
  vaddr_t va = (vaddr_t)p;
  paddr_t paddr = (paddr_t)rcr3();
  pte_t *ppte;

#define WALK_LEVEL(LVL) \
    ppte = &((pte_t *)paddr)[PT_LEVEL_OFFSET(va, LVL)]; \
    if (!FLAG_ISSET(*ppte, PG_V) \
        || (LVL != PT_L1 && FLAG_ISSET(*ppte, PG_PS)) \
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

paddr_t get_paddr_page(void *p, OUT enum pt_level *page_level) {
  pte_t *ppte;
  enum pt_level level = pt_walk(p, PGWALK_FULL, OUT &ppte);

  if (!FLAG_ISSET(*ppte, PG_V))
    return 0;

  if (page_level)
    OUT *page_level = level;

  paddr_t pa;
  switch (level) {
  case PT_4K:
    return (*ppte & PG_FRAME);

  case PT_2M:
    assert(FLAG_ISSET(*ppte, PG_PS));
    return (*ppte & PG_2MFRAME);

  case PT_1G:
    assert(FLAG_ISSET(*ppte, PG_PS));
    return (*ppte & PG_1GFRAME);
  }

  UNREACHABLE("Unhandled pt_level value!\n");
}

size_t get_page_offset(uintptr_t addr, enum pt_level pt_level) {
  uintptr_t page_offset_mask = (1 << (PGSHIFT + (pt_level - 1) * 9)) - 1;
  return (size_t)(addr & page_offset_mask);
}

paddr_t get_paddr(void *p) {
  enum pt_level level;
  paddr_t pa = get_paddr_page(p, OUT &level);
  if (!pa)
    return 0;

  return pa + get_page_offset((uintptr_t)p, level);
}

int pt_map(paddr_t pa, vaddr_t va, pte_t flags) {
  assert(pa % PGSIZE == 0);

  paddr_t ptpa;
  pte_t *ppte;
  enum pt_level level = pt_walk((void *)va, PGWALK_FULL, OUT &ppte);

  #define CREATE_LEVEL(LVL) \
    ptpa = pp_zalloc(1); \
    if (!ptpa) { \
      VIRTMEM_DPRINTF("pt_map failed: failed to allocate pagetable page"); \
      /* TODO: clean-up */ \
      return -1; \
    } \
    *ppte = (pte_t)ptpa | flags | PG_V; \
    ppte = &((pte_t *)paddr2vaddr(ptpa))[PT_LEVEL_OFFSET(va, LVL)]

  switch (level) {
  case PT_L4: CREATE_LEVEL(3);
  case PT_L3: CREATE_LEVEL(2);
  case PT_L2: CREATE_LEVEL(1);
  }

  *ppte = pa | flags | PG_V;
  return 0;
}

int pt_unmap(vaddr_t va, enum pt_level on_level) {
  pte_t *ppte;
  enum pt_level level = pt_walk((void *)va, on_level, OUT &ppte);
  if (level != on_level) {
    VIRTMEM_DPRINTF("pt_unmap failed: 0x%lx is not mapped under level %u, pt_walk() returned level %u\n", va, on_level, level);
    return -1;
  }

  *ppte = 0;
  return 0;
}