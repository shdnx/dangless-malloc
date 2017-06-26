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

uint64_t rcr3(void) {
  uint64_t val;
  asm("movq %%cr3, %0" : "=r" (val));
  return val;
}

typedef uint64_t pte_t;

typedef uintptr_t paddr_t;
typedef uintptr_t vaddr_t;

//#define PGSHIFT 12
//#define PGSIZE (1uL << PGSHIFT)

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

enum pt_level pt_walk(void *p, enum pt_level requested_level, OUT pte_t **result_ppte) {
  LOG("page walking %p for level %u\n", p, requested_level);
  vaddr_t va = (vaddr_t)p;
  paddr_t paddr = (paddr_t)rcr3();
  pte_t *ppte;

#define WALK_LEVEL(LVL) \
    LOG("Level " #LVL "... paddr = 0x%lx; pte index = %zu\n", paddr, pt_level_offset(va, LVL)); \
    ppte = &((pte_t *)paddr)[pt_level_offset(va, LVL)]; \
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

static int idmap(void *base, paddr_t start, size_t len) {
  // I think this is necessary to update the EPT
  void *result = mmap(base, len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (result == MAP_FAILED) {
    LOG("failed for %p!\n", base);
    return 1;
  }

  if (result != base) {
    LOG("didn't get what expected: base = %p, result = %p\n", base, result);
    return 2;
  }

  // map the requested physical memory to it
  return dune_vm_map_phys(pgroot, base, len, (void *)start, PERM_R | PERM_W);
}

static void fixpgmapping(void *p) {
  void *result = mmap(p, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (result == MAP_FAILED) {
    LOG("failed for %p!\n", p);
    return;
  }

  if (result != p) {
    LOG("didn't get what expected: p = %p, result = %p\n", p, result);
    return;
  }

  pte_t *ppte;
  enum pt_level level = pt_walk(p, PT_L1, OUT &ppte);
  LOG("Got level %u PPTE: %p\n", level, ppte);
  LOG("PTE = 0x%lx\n", *ppte);

  dune_vm_map_phys(pgroot, p, PGSIZE, (void *)dune_va_to_pa(p), PERM_R | PERM_W);
  LOG("success for %p\n", p);
}

void dump_pt(FILE *os, pte_t *pt, enum pt_level level) {
  unsigned level_shift = pt_level_shift(level);

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    pte_t pte = pt[i];
    if (pte) {
      fprintf(os, "Mapped: [%zu] 0x%lx - 0x%lx by ", i, (uintptr_t)i << level_shift, (uintptr_t)(i + 1) << level_shift);
      dump_pte(os, pte, level);

      if (level == PT_L4) {
        // NOTE: causes EPT violation with - PAGEBASE; pagefault without the PAGEBASE, or with + PAGEBASE
        uint64_t ptl3_pa = (pte & PTE_FRAME) - PAGEBASE;
        ptl3_pa += 0xA000000000uL;
        //fixpgmapping((void *)ptl3_pa);

        //dune_vm_map_phys(pgroot, (void *)ptl3_pa, PGSIZE, (void *)dune_va_to_pa((void *)ptl3_pa), PERM_R | PERM_W);

        fprintf(os, " --- PTL3 ---\n");
        dump_pt(os, (pte_t *)ptl3_pa, PT_L3);
        fprintf(os, " --- /PTL3 ---\n");
      }
    }
  }
}

static void pgflt_handler(uintptr_t addr, uint64_t fec, struct dune_tf *tf) {
  LOG("ERROR pagefault for addr = 0x%lx\n", addr);

  //LOG("err = 0x%lx, rflags = 0x%lx\n", tf->err, tf->rflags);
  // NOTE: attempting to dump the stack in dune_dump_trap_frame() causes a triplefault
  //dune_dump_trap_frame(tf);
  dune_procmap_dump();

  static uintptr_t s_last_addr = 0;
  if (s_last_addr == addr) {
    LOG("Fatal pagefault, exiting!\n");
    exit(-1);
  }

  s_last_addr = addr;

  // ---

  ptent_t *pte;
  int ret = dune_vm_lookup(pgroot, (void *) addr, CREATE_NORMAL, &pte);
  assert(!ret);
  *pte = PTE_P | PTE_W | PTE_ADDR(dune_va_to_pa((void *) addr));
}

int main_dump_pml4(int argc, const char **argv) {
  dune_register_pgflt_handler(&pgflt_handler);

  // NOTE: this also fails with the same pgfault as everything else, WHY THE FUCK
  // perhaps try with own pt_map() instead of Dune's shit?
  // or maybe when entering Dune, ask for map_full?
  LOG("doing idmap...\n");
  idmap((void *)0xA000000000uL, 0, 1uL << 32);
  LOG("idmap done!\n");

  LOG("pgroot (cr3/pml4) = %p\n", pgroot);
  dump_pt(stderr, pgroot, PT_L4);
  return 0;
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

int main_dump_pgroot_region(int argc, const char **argv) {
  dune_register_pgflt_handler(&pgflt_handler);

  LOG("pgroot (cr3/pml4) = %p\n", pgroot);

  vaddr_t base = (vaddr_t)pgroot - PGSIZE;
  size_t i;
  for (i = 1; i < 35; i++) { // 35 max
    LOG("--- Page %zu:\n", i);
    dump_pt(stderr, (ptent_t *)(base + i * PGSIZE), PT_L4);
  }

  pgallocwrite();

  LOG(" ------------------- AGAIN ----------------\n");

  for (i = 1; i < 35; i++) { // 35 max
    LOG("--- Page %zu:\n", i);
    dump_pt(stderr, (ptent_t *)(base + i * PGSIZE), PT_L4);
  }

  dune_procmap_dump();
  return 0;
}

int main_ptmapping(int argc, const char **argv) {
  main_dump_pml4(argc, argv);

  uint64_t *p = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  ASSERT(p != MAP_FAILED, "mmap error\n");

  LOG("p = %p\n", p);
  *p = 0xDEADBEEF;
  LOG("Wrote value: 0x%lx\n", *p);

  paddr_t ppa = dune_va_to_pa(p);
  LOG("pa = 0x%lx\n", ppa);

  pte_t *ppte;
  enum pt_level level = pt_walk((void *)ppa, PT_L1, OUT &ppte);
  LOG("Got level %u PPTE: %p\n", level, ppte);
  LOG("PTE = 0x%lx\n", *ppte);

  return 0;
}

int main2(int argc, const char **argv) {
  int result;
  uint64_t *p = mmap(NULL, 512, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  ASSERT(p != MAP_FAILED, "mmap error\n");

  LOG("mmap addr: %p\n", p);

  uintptr_t pa = dune_mmap_addr_to_pa(p);
  LOG("dune_mmap_addr_to_pa: %lx\n", pa);

  // int dune_vm_lookup(ptent_t *root, void *va, int create, ptent_t **pte_out)
  ptent_t *pte = NULL;
  if ((result = dune_vm_lookup(pgroot, p, 0, OUT &pte)) < 0) {
    LOG("dune_vm_lookup failed (pte at %p)!\n", pte);
  } else {
    LOG("dune_vm_lookup => pte at %p, value: 0x%lx\n", pte, *pte);
  }

  *p = 0xBEEFBABEuL;
  LOG("Wrote value: %lx\n", *p);

  pte = NULL;
  /*if ((result = dune_vm_lookup(pgroot, p, 0, OUT &pte)) < 0) {
    LOG("dune_vm_lookup2 failed (pte at %p)!\n", pte);
  } else {
    LOG("dune_vm_lookup2 => pte at %p, value: 0x%lx\n", pte, *pte);
  }*/

  /*void *remap = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  ASSERT(remap, "mmap error on remap\n");

  uintptr_t pa2 = dune_mmap_addr_to_pa(remap);
  LOG("mmap2: va = %p, pa = %lx\n", remap, pa2);*/

  /*for (int i = 0; i < 100; i++) {
    void *ptr = dune_mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) {
      LOG("mmap %d failed!\n");
      break;
    }

    LOG("mmap %d => %p\n", i, ptr);
    LOG("\tPA = 0x%lx\n", dune_mmap_addr_to_pa(ptr));
  }*/

  /*result = dune_vm_map_phys(pgroot, remap, PGSIZE, (void *)pa, PERM_R | PERM_W);
  ASSERT(result == 0, "dune_vm_map_phys error!\n");

  uint64_t *remapped_p = (uint8_t *)remap + get_page_offset(p);
  LOG("Reading remapped value...\n");
  LOG("Read remapped value: %lx\n", *remapped_p);*/

  return 0;
}

#define EFFECTIVE_MAIN main_dump_pml4

int main(int argc, const char **argv) {
  int result = dune_init_and_enter();
  ASSERT(result == 0, "Failed to enter Dune!\n");

  return EFFECTIVE_MAIN(argc, argv);
}