#if DANGLESS_CONFIG_SUPPORT_MULTITHREADING
  #include <pthread.h>
#else
  #include "dangless/pthread_mock.h"
#endif

#include "dangless/common.h"
#include "dangless/common/statistics.h"
#include "dangless/virtmem.h"
#include "dangless/platform/physmem_alloc.h"

#if DANGLESS_CONFIG_DEBUG_VIRTMEM
  #define LOG(...) vdprintf(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

STATISTIC_DEFINE_COUNTER(st_num_pagetables_allocated);

static pthread_mutex_t g_pt_mapping_mutex = PTHREAD_MUTEX_INITIALIZER;

enum pt_level pt_walk(void *p, enum pt_level requested_level, OUT pte_t **result_ppte) {
  vaddr_t va = (vaddr_t)p;
  paddr_t paddr = (paddr_t)rcr3();
  pte_t *ppte;

#define WALK_LEVEL(LVL) \
    ppte = &((pte_t *)pt_paddr2vaddr(paddr))[pt_level_offset(va, LVL)]; \
    if (!FLAG_ISSET(*ppte, PTE_V) \
        || (LVL != PT_L1 && FLAG_ISSET(*ppte, PTE_PS)) \
        || requested_level == LVL) { \
      OUT *result_ppte = ppte; \
      return LVL; \
    } \
    paddr = (paddr_t)(*ppte & PTE_FRAME)

  WALK_LEVEL(PT_L4);
  WALK_LEVEL(PT_L3);
  WALK_LEVEL(PT_L2);
  WALK_LEVEL(PT_L1);

#undef WALK_LEVEL

  // TODO: I think this is actually unreachable code?
  OUT *result_ppte = ppte;
  return 1;
}

paddr_t pt_resolve_page(void *p, OUT enum pt_level *page_level) {
  pte_t *ppte;
  enum pt_level level = pt_walk(p, PGWALK_FULL, OUT &ppte);

  if (!FLAG_ISSET(*ppte, PTE_V))
    return 0;

  if (page_level)
    OUT *page_level = level;

  switch (level) {
  case PT_4K:
    return (*ppte & PTE_FRAME_4K);

  case PT_2M:
    ASSERT0(FLAG_ISSET(*ppte, PTE_PS));
    return (*ppte & PTE_FRAME_2M);

  // TODO: does this make sense?
  case PT_1G:
    ASSERT0(FLAG_ISSET(*ppte, PTE_PS));
    return (*ppte & PTE_FRAME_1G);

  default:
    UNREACHABLE("Unhandled pt_level value!\n");
  }
}

paddr_t pt_resolve(void *p) {
  enum pt_level level;
  paddr_t pa = pt_resolve_page(p, OUT &level);
  if (!pa)
    return 0;

  return pa + get_page_offset((uintptr_t)p, level);
}

static int pt_map_page_create_level(
  REF pte_t **pppte,
  vaddr_t va,
  enum pt_level level,
  enum pte_flags flags
) {
  paddr_t ptpa = pp_zalloc_one();
  if (!ptpa) {
    LOG("failed to allocate pagetable page!\n");
    return -1;
  }

  STATISTIC_UPDATE() {
    st_num_pagetables_allocated++;
  }

  **pppte = (pte_t)ptpa | flags | PTE_V;
  REF *pppte = &((pte_t *)pt_paddr2vaddr(ptpa))[pt_level_offset(va, level)];

  return 0;
}

int pt_map_page(paddr_t pa, vaddr_t va, enum pte_flags flags) {
  ASSERT0(pa % PGSIZE == 0);
  ASSERT0(va % PGSIZE == 0);

  LOG("mapping physical page %p to virtual %p with flags %d\n", (void *)pa, (void *)va, flags);

  // NOTE: ideally, we would do some more fine-grained locking that would allow unrelated mappings to happen in parallel
  // this would be especially important with having thread-local virtual memory regions dedicated to threads that they can use for remapping
  // for instance, each PML4 entry could have its own mutex
  // actually, a spinlock would probably work better
  pthread_mutex_lock(&g_pt_mapping_mutex);

  paddr_t ptpa;
  pte_t *ppte;
  enum pt_level level = pt_walk((void *)va, PGWALK_FULL, OUT &ppte);

  ASSERT(!FLAG_ISSET(*ppte, PTE_V), "Attempted to replace a hugepage mapping for VA %p at %p (PTE: 0x%lx) with a 4K page mapping to %p!", (void *)va, ppte, *ppte, (void *)pa);

#define CREATE_LEVEL(LVL) \
  do { \
    if ((pt_map_page_create_level(REF &ppte, va, LVL, flags)) < 0) \
      goto fail_cleanup; \
  } while (0)

  switch (level) {
  case PT_L4: CREATE_LEVEL(PT_L3);
  case PT_L3: CREATE_LEVEL(PT_L2);
  case PT_L2: CREATE_LEVEL(PT_L1);
  }

#undef CREATE_LEVEL

  // flush the TLB entry if we're overwriting an entry
  if (FLAG_ISSET(*ppte, PTE_V)) {
    tlb_flush_one((void *)va);
  }

  *ppte = pa | flags | PTE_V;

  pthread_mutex_unlock(&g_pt_mapping_mutex);
  return 0;

fail_cleanup:
  // TODO: clean-up
  pthread_mutex_unlock(&g_pt_mapping_mutex);
  return -1;
}

int pt_map_region(paddr_t pa, vaddr_t va, size_t size, enum pte_flags flags) {
  ASSERT0(pa % PGSIZE == 0);
  ASSERT0(va % PGSIZE == 0);
  ASSERT0(size % PGSIZE == 0);

  LOG("mapping physical region %p to virtual region %p of size %zu with flags %d\n", (void *)pa, (void *)va, size, flags);

  // TODO: can use huge-page mapping transparently here if pa, va and size are all multiplies of 2 MB

  // TODO: this could be optimized, since usually we'll likely be working with neighbouring PTEs
  int result = 0;
  size_t offset;
  size_t max_offset;

  for (offset = 0; offset < size; offset += PGSIZE) {
    if ((result = pt_map_page(pa + offset, va + offset, flags)) < 0) {
      LOG("could not map page offset 0x%lx\n", offset);
      goto fail_unmap;
    }
  }

  return 0;

fail_unmap:
  max_offset = offset;
  for (offset = 0; offset < max_offset; offset += PGSIZE) {
    // an error may occur here, but we can't do anything about that, so we'll just ignore it
    pt_unmap_page(va + offset, PT_4K);
  }

  return -1;
}

int pt_unmap_page(vaddr_t va, enum pt_level on_level) {
  pte_t *ppte;
  enum pt_level level = pt_walk((void *)va, on_level, OUT &ppte);
  if (level != on_level) {
    LOG("0x%lx is not mapped under level %u, pt_walk() returned level %u\n", va, on_level, level);
    return -1;
  }

  pte_t old_pte = *ppte;
  *ppte = 0;

  if (FLAG_ISSET(old_pte, PTE_V))
    tlb_flush_one((void *)va);

  return 0;
}
