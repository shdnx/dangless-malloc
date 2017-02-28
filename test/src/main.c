#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arch/amd64/include/pte.h>

#include "common.h"
#include "virtmem.h"
#include "safemalloc.h"

#define PRINTF_INDENT(INDENT_LENGTH, FORMAT, ...) \
  printf("%*s" FORMAT, (INDENT_LENGTH), "", __VA_ARGS__)

// TODO: can do bigger jumps if the PTE in PTL4, 3 or 2 are non-existant
static void dump_mappings(uintptr_t va_start, uintptr_t va_end) {
  uintptr_t va;
  paddr_t pa;
  unsigned level;

  bool mapped = false;
  uintptr_t start = 0;

  for (va = va_start; va < va_end; va += PGSIZE) {
    pa = get_paddr_page((void *)va, OUT &level);
    if (pa) {
      if (!mapped && start != 0) {
        printf("Unmapped region 0x%lx - 0x%lx\n", start, va);
        start = 0;
      }

      if (pa == va) {
        mapped = true;
        if (start == 0) {
          start = va;
        }
      } else {
        if (level == 2 && (va >> L2_SHIFT) == (pa >> L2_SHIFT)) {
          // 2 MB large-page
          mapped = true;
          if (start == 0) {
            start = va - PGSIZE;
          }

          va += 2 * MB - 2 * PGSIZE;
          continue;
        }

        printf("Non-identity mapping: VA 0x%lx => PA 0x%lx\n", va, pa);
      }
    } else {
      if (mapped && start != 0) {
        printf("Identity-mapped region 0x%lx - 0x%lx\n", start, va);
        start = 0;
      }

      mapped = false;
      if (start == 0) {
        start = va;
      }
    }
  }

  if (start != 0) {
    printf("Final %s region 0x%lx - 0x%lx\n", mapped ? "identity-mapped" : "unmapped", start, va);
  }
}

static unsigned get_level_shift(unsigned level) {
  switch (level) {
  case 4: return L4_SHIFT;
  case 3: return L3_SHIFT;
  case 2: return L2_SHIFT;
  case 1: return L1_SHIFT;
  default: __builtin_unreachable();
  }
}

static void dump_pt(pte_t *pt, unsigned level) {
  unsigned level_shift = get_level_shift(level);

  size_t i;
  for (i = 0; i < 512; i++) {
    pte_t pte = pt[i];
    if (pte) {
      printf("Mapped: 0x%lx - 0x%lx by ", (uintptr_t)i << level_shift, (uintptr_t)(i + 1) << level_shift);
      pte_dump(stdout, pte, level);
    }
  }
}

static void dump_pt_summary(pte_t *pt, unsigned level) {
  unsigned level_shift = get_level_shift(level);

  bool mapped = false;
  size_t count = 0;

  size_t i;
  for (i = 0; i < 512; i++) {
    pte_t pte = pt[i];
    if (pte) {
      if (!mapped && count > 0) {
        printf("Unmapped: 0x%lx - 0x%lx [%zu entries]\n",
          (uintptr_t)(i - count) << level_shift,
          (uintptr_t)i << level_shift,
          count);

        count = 0;
      }

      mapped = true;
      count++;
    } else {
      if (mapped && count > 0) {
        printf("Mapped: 0x%lx - 0x%lx [%zu entries]\n",
          (uintptr_t)(i - count) << level_shift,
          (uintptr_t)i << level_shift,
          count);

        count = 0;
      }

      mapped = false;
      count++;
    }
  }

  if (count > 0) {
    printf("%s: 0x%lx - 0x%lx [%zu entries]\n",
      mapped ? "Mapped" : "Unmapped",
      (uintptr_t)(i - count) << level_shift,
      (uintptr_t)i << level_shift,
      count);
  }
}

static void *scan_memory(void *start_, void *end_, uint8_t *pattern, size_t pattern_length, size_t pattern_align) {
  uint8_t *start = (uint8_t *)start_;
  uint8_t *end = (uint8_t *)end_;
  size_t length = end - start;

  fprintf(stderr, "Scanning memory from %p to %p...\n", start, end);

  unsigned prev_percent = 0;

  uint8_t *p;
  for (p = start; p != end; p += pattern_align) {
    if (memcmp(p, pattern, pattern_length) == 0)
      return p;

    unsigned curr_percent = (unsigned)(((double)(p - start)) / ((double)length) * 100);
    if (prev_percent != curr_percent) {
      prev_percent = curr_percent;
      fprintf(stderr, "%u%%... ", curr_percent);
    }
  }

  return NULL;
}

static int global_var;

#include "rumprun.h"
RUMPRUN_DEF_FUNC(0xcac0, void, bmk_pgalloc_dumpstats, void);

int main() {
  printf("Hello, Rumprun!\n");

  //RUMPRUN_FUNC(bmk_pgalloc_dumpstats)();

  // memory scanning
  //const char *symname = "bmk_pgalloc";
  //uint8_t pattern[] = { 0xbe, 0x00, 0x10, 0x00, 0x00, 0xe9, 0x06, 0xfe, 0xff, 0xff, 0x66, 0x0f, 0x1f, 0x44, 0x00, 0x00 };
  //const size_t pattern_length = 16;

  /*const char *symname = "pgalloc_totalkb";
  unsigned long pattern = 3208; // pgalloc_totalkb = 60680
  const size_t pattern_length = sizeof(pattern);

  void *p = scan_memory(
    (void *)(KB * 4),
    (void *)(GB * 1),
    (uint8_t *)&pattern,
    pattern_length,
    pattern_length);
  fprintf(stderr, "\n");

  if (!p) {
    printf("Could not find %s! :(\n", symname);
  } else {
    printf("%s = %p\nNext 32 bytes:\n", symname, p);

    size_t i;
    for (i = 0; i < 32; i++) {
      printf("0x%hhx ", ((uint8_t *)p)[pattern_length + i]);
    }
    printf("\n");

    printf("Previous 32 bytes:\n");
    for (; i > 1; i--) {
      printf("0x%hhx ", ((uint8_t *)p)[pattern_length - i]);
    }
    printf("\n");
  }*/

  // pgalloc_totalkb = 0x4929a8 ??
  // pgalloc_usedkb = 0x492a68 ??

  // -------------

  uint64_t cr3 = rcr3();
  printf("cr3 = 0x%lx\n", cr3);

  pte_t *pml4 = (pte_t *)cr3;

  printf("pml4 = %p, phys = 0x%lx\n", pml4, get_paddr(pml4));
  printf("&pml4 = %p, phys = 0x%lx\n", &pml4, get_paddr(&pml4));
  printf("&main = %p, phys = 0x%lx\n", &main, get_paddr(&main));
  printf("&global_var = %p, phys = 0x%lx\n", &global_var, get_paddr(&global_var));

  void *mallocd = malloc(sizeof(void *));
  printf("mallocd = %p, phys = 0x%lx\n", mallocd, get_paddr(mallocd));
  free(mallocd);

  void *safemallocd = safe_malloc(sizeof(void *));
  printf("safemallocd = %p, phys = 0x%lx\n", safemallocd, get_paddr(safemallocd));
  safe_free(safemallocd);

  // -------------

  // user space
  // 0x0000000000001000 - 0x0000000100000000: identity mapping (first 4 GB)
  // 0x0000000100000000 - 0x00007f8000000000: unmapped
  //dump_mappings(PAGE_SIZE, 0x00007f8000000000);

  // kernel space: entirely unmapped??
  //dump_mappings(0xffff800000000000, 0xffffff8000000000);

  // PML4 only covers 0x0 - 0x8000000000
  // PTL3 at PML4[0] is mapped for the first 4 entries (until 4 GB)
  // PTL2 at PML4[0][3] is fully mapped
  // so the only mapped part of the address space is the first 4 GBs
  /*printf("PML4:\n");
  dump_pt(pml4, 4);

  printf("\nPTL3 at PML4[0]:\n");
  pte_t *ptl3_at_0 = (pte_t *)(pml4[0] & PG_FRAME);
  dump_pt_summary(ptl3_at_0, 3);

  printf("\nPTL2 at PML4[0][3]:\n");
  pte_t *ptl2_at_03 = (pte_t *)(ptl3_at_0[3] & PG_FRAME);
  //dump_pt(ptl2_at_03, 2);
  dump_pt_summary(ptl2_at_03, 2);*/

  return 0;
}
