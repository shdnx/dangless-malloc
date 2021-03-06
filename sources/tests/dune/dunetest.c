#define _GNU_SOURCE

#include <stdlib.h> // abort
#include <stdio.h>

#include <errno.h> // perror

#include <dangless/virtmem.h>

#include "dunetest.h"

/*static uintptr_t g_checking_memregion_start = 0;
static uintptr_t g_checking_memregion_end = 0;

void dunetest_check_memregion(void *start, size_t len) {
  g_checking_memregion_start = (uintptr_t)start;
  g_checking_memregion_end = (uintptr_t)start + len;
}

static void pagefault_handler(uintptr_t addr, ?? err, struct dune_tf *tf) {
  if (g_checking_memregion_start <= addr && addr <= g_checking_memregion_end) {

  }
}*/

void dunetest_init(void) {
  static bool s_initalized = false;
  if (s_initalized)
    return;

  s_initalized = true;

  if (!in_kernel_mode()) {
    fprintf(stderr, "Entering Dune...\n");

    if (dune_init_and_enter() != 0) {
      perror("Failed to enter Dune mode");
      abort();
    }

    fprintf(stderr, "Now in Dune!\n");
  } else {
    fprintf(stderr, "Was already in Dune mode, won't try to enter again\n");
  }

  //dune_register_pgflt_handler(&pagefault_handler);

  fprintf(stderr, "Init done, starting tests...\n");
}
