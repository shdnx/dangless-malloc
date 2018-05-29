#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dangless/common.h"
#include "dangless/virtmem.h"
#include "dangless/dangless_malloc.h"
#include "dangless/dump.h"
#include "dangless/platform/sysmalloc.h"

static int global_var;

int main() {
  printf("Hello, Rumprun!\n");

  printf("&malloc = %p, &dangless_malloc = %p\n", &malloc, &dangless_malloc);

  /*uint64_t cr3 = rcr3();
  printf("cr3 = 0x%lx\n", cr3);

  pte_t *pml4 = (pte_t *)cr3;

  printf("pml4 = %p, phys = 0x%lx\n", pml4, pt_resolve(pml4));
  printf("&pml4 = %p, phys = 0x%lx\n", &pml4, pt_resolve(&pml4));
  printf("&main = %p, phys = 0x%lx\n", &main, pt_resolve(&main));
  printf("&global_var = %p, phys = 0x%lx\n", &global_var, pt_resolve(&global_var));*/

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
