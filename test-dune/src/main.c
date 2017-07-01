#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "dune.h"

#include "common.h"

#define LOG(...) vdprintf(__VA_ARGS__)

static size_t get_page_offset(void *addr) {
  return (uintptr_t)addr % PGSIZE;
}

// Note: there doesn't seem to be a limit to the amount of virtual memory we can allocate. I let it run to 192 GB (50331648 pages).
int main_virtmemlimit(int argc, const char **argv) {
  size_t counter = 0;
  while (true) {
    uint64_t *p = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (p == MAP_FAILED)
      break;

    counter++;

    if (counter % (1024 * 1024) == 0) {
      LOG("%lu... %p\n", counter, p);
    }
  }

  LOG("Final counter: %lu", counter);
  return 0;
}

typedef uint64_t pte_t;

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

#define PT_BITS_PER_LEVEL 9
#define PT_NUM_ENTRIES (1uL << PT_BITS_PER_LEVEL)

enum pte_flags {
  PTE_V = 0x1, // valid
  /*PTE_W = 0x2, // writable
  PTE_U = 0x4, // user accessible
  PTE_PS = 0x80, // page size (everywhere but in PTL1)
  PTE_PAT = 0x80, // PAT (on PTE)
  PTE_NX = 0x8000000000000000uL*/ // non-executable
};

enum {
  PTE_FRAME = 0x000ffffffffff000uL
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

static inline unsigned pt_level_shift(enum pt_level level) {
  return PGSHIFT + (level - 1) * PT_BITS_PER_LEVEL;
}

static inline size_t pt_level_offset(vaddr_t va, enum pt_level level) {
  return (va >> pt_level_shift(level)) & (PT_NUM_ENTRIES - 1);
}

void dump_pte(FILE *os, pte_t pte, enum pt_level level) {
  fprintf(os, "PTE addr = 0x%lx", pte & PTE_FRAME);

#define HANDLE_BIT(BIT) if (pte & (BIT)) fprintf(os, " " #BIT)

  HANDLE_BIT(PTE_V);
  HANDLE_BIT(PTE_W);
  HANDLE_BIT(PTE_U);
  HANDLE_BIT(PTE_NX);

  if (level == PT_L1)
    HANDLE_BIT(PTE_PAT);
  else
    HANDLE_BIT(PTE_PS);

#undef HANDLE_BIT

  fprintf(os, " (raw: 0x%lx)\n", pte);
}

#if 0
enum pt_level pt_walk(void *p, enum pt_level requested_level, OUT pte_t **result_ppte) {
  LOG("page walking %p for level %u\n", p, requested_level);
  vaddr_t va = (vaddr_t)p;
  paddr_t paddr = (paddr_t)rcr3();
  pte_t *ppte;

#define WALK_LEVEL(LVL) \
    LOG("Level " #LVL "... paddr = 0x%lx; pte index = %zu\n", paddr, pt_level_offset(va, LVL)); \
    ppte = &((pte_t *)???(paddr))[pt_level_offset(va, LVL)]; \
    if (!FLAG_ISSET(*ppte, PTE_V) \
        || (LVL != PT_L1 && FLAG_ISSET(*ppte, PTE_PS)) \
        || requested_level == LVL) { \
      OUT *result_ppte = ppte; \
      return LVL; \
    } \
    dump_pte(stderr, *ppte, LVL); \
    paddr = (paddr_t)(*ppte & PTE_FRAME)

  WALK_LEVEL(PT_L4);
  WALK_LEVEL(PT_L3);
  WALK_LEVEL(PT_L2);
  WALK_LEVEL(PT_L1);

#undef WALK_LEVEL

  OUT *result_ppte = ppte;
  return 1;
}
#endif

void dump_pt(FILE *os, pte_t *pt, enum pt_level level) {
  unsigned level_shift = pt_level_shift(level);

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    pte_t pte = pt[i];
    if (pte) {
      fprintf(os, "[%zu] 0x%lx - 0x%lx by ", i, (uintptr_t)i << level_shift, (uintptr_t)(i + 1) << level_shift);
      dump_pte(os, pte, level);
    }
  }
}

static void pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf) {
  LOG("ERROR pagefault for addr = 0x%lx\n", addr);

  //dune_procmap_dump();
  dune_dump_trap_frame(tf);

  static uintptr_t s_last_addr = 0;
  if (s_last_addr == addr) {
    LOG("Fatal pagefault, exiting!\n");
    exit(-1);
  }

  s_last_addr = addr;

  // try to demand-page things - seems pointless currently
  /*ptent_t *pte;
  int ret = dune_vm_lookup(pgroot, (void *) addr, CREATE_NORMAL, &pte);
  assert(!ret);
  *pte = PTE_P | PTE_W | PTE_ADDR(dune_va_to_pa((void *) addr));*/
}

static void *pgallocwrite() {
  uint64_t *p = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  ASSERT(p != MAP_FAILED, "mmap error\n");

  LOG("p = %p\n", p);
  *p = 0xDEADBEEF;
  LOG("Wrote value: 0x%lx\n", *p);

  paddr_t ppa = dune_va_to_pa(p);
  LOG("pa = 0x%lx\n", ppa);
  return p;
}

int main_info(int argc, const char **argv) {
  dune_register_pgflt_handler(&pgflt_handler);

  LOG(" --- PML4 ---\n");
  dump_pt(stderr, pgroot, PT_L4);
  fprintf(stderr, "\n");

  dune_procmap_dump();
  fprintf(stderr, "\n");

  LOG("phys_limit = 0x%lx\n", phys_limit);
  LOG("mmap_base  = 0x%lx\n", mmap_base);
  LOG("stack_base = 0x%lx\n", stack_base);

#if 0
  unsigned level_shift = pt_level_shift(PT_L4);

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    pte_t pte = pgroot[i];
    if (!pte)
      continue;

    /*const uintptr_t start_va = (uintptr_t)i << level_shift;
    const uintptr_t end_va = (uintptr_t)(i + 1) << level_shift;

    LOG("Mapped: [%zu] 0x%lx - 0x%lx by ", i, start_va, end_va);
    dump_pte(stderr, pte, level);*/

    paddr_t ptl3_pa = (pte & PTE_FRAME);
    /*if (ptl3_pa >= GB4) {
      LOG("PTL3 PA 0x%lx is above the 4 GB limit 0x%lx!\n", ptl3_pa, GB4);
    } else{*/
      pte_t *ptl3_va = (pte_t *)ptl3_pa;
      dump_pt(stderr, ptl3_va, PT_L3);
    //}
  }
#endif

  return 0;
}

static void analyze_ptr(uint64_t *p) {
  LOG("p = %p\n", p);
  LOG("dune_va_to_pa => 0x%lx\n", dune_va_to_pa((uintptr_t)p));

  ptent_t *ppte;
  if (dune_vm_lookup(pgroot, p, 0, &ppte) == 0) {
    dump_pte(stderr, *ppte, PT_L1);
  } else {
    LOG("dune_vm_lookup failed!\n");
  }

  LOG("Has uninitialized value: 0x%lx\n", *p);
  *p = 0xDEADBEEF;
  LOG("Wrote value: 0x%lx\n", *p);
}

int main_malloc_test(int argc, const char **argv) {
  main_info(argc, argv);

  fprintf(stderr, "\n---- MALLOC ----\n\n");

  uint64_t *p = malloc(sizeof(uint64_t));
  ASSERT(p, "malloc failed!\n");

  analyze_ptr(p);

  fprintf(stderr, "\n---- MMAP ----\n\n");

  uint64_t *p2 = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  ASSERT(p2 != MAP_FAILED, "mmap error\n");

  analyze_ptr(p2);

  return 0;
}

#define EFFECTIVE_MAIN main_malloc_test

int main(int argc, const char **argv) {
  int result = dune_init_and_enter();
  ASSERT(result == 0, "Failed to enter Dune!\n");

  return EFFECTIVE_MAIN(argc, argv);
}