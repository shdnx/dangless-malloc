#include <stdio.h>
#include <stdlib.h>

// dangless stuff
#include "dangless_malloc.h"
#include "virtmem.h"
#include "platform/sysmalloc.h"

// dune stuff
#include "libdune/dune.h"

int main() {
  int result = dune_init_and_enter();
  ASSERT(result == 0, "Failed to enter Dune!\n");

  void *mallocd = MALLOC(void *);
  printf("mallocd = %p, phys = 0x%lx\n", mallocd, pt_resolve(mallocd));
  FREE(mallocd);

  // pml4[1]
  dangless_dedicate_vmem((void *)0x8000000000uL, (void *)(0x8000000000uL * 2));

  void *safemallocd1 = dangless_malloc(sizeof(void *));
  printf("safemallocd1 = %p, phys = 0x%lx\n", safemallocd1, pt_resolve(safemallocd1));
  dangless_free(safemallocd1);
  printf("safemallocd1 freed!\n");

  void *safemallocd2 = malloc(sizeof(void *));
  printf("overriden safemallocd2 = %p, phys = 0x%lx\n", safemallocd2, pt_resolve(safemallocd2));
  free(safemallocd2);
  printf("safemallocd2 freed!\n");

  return 0;
}
