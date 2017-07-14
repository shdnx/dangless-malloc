#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "virtmem.h"
#include "dangless_malloc.h"
#include "dump.h"

#include "rumprun.h"
RUMPRUN_DEF_FUNC(0xcac0, void, bmk_pgalloc_dumpstats, void);

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

  printf("pml4 = %p, phys = 0x%lx\n", pml4, pt_resolve(pml4));
  printf("&pml4 = %p, phys = 0x%lx\n", &pml4, pt_resolve(&pml4));
  printf("&main = %p, phys = 0x%lx\n", &main, pt_resolve(&main));
  printf("&global_var = %p, phys = 0x%lx\n", &global_var, pt_resolve(&global_var));

  void *mallocd = MALLOC(void *);
  printf("mallocd = %p, phys = 0x%lx\n", mallocd, pt_resolve(mallocd));
  FREE(mallocd);

  dangless_dedicate_vmem((void *)(GB * 5), (void *)(GB * 1024));

  void *safemallocd1 = dangless_malloc(sizeof(void *));
  printf("safemallocd1 = %p, phys = 0x%lx\n", safemallocd1, pt_resolve(safemallocd1));
  dangless_free(safemallocd1);
  printf("safemallocd1 freed!\n");

  void *safemallocd2 = malloc(sizeof(void *));
  printf("overriden safemallocd2 = %p, phys = 0x%lx\n", safemallocd2, pt_resolve(safemallocd2));
  free(safemallocd2);
  printf("safemallocd2 freed!\n");

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
