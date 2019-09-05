#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "dune.h"

static void dune_autoenter(void) {
  int result;
  if ((result = dune_init(/*map_full=*/true)) != 0) {
    perror("[dune-autoenter] Failed to initialize Dune");
    abort();
  }

  if ((result = dune_enter()) != 0) {
    perror("[dune-autoenter] Failed to enter Dune mode");
    abort();
  }

#if DEBUG
  fprintf(stderr, "[dune-autoenter] Now running in Dune mode!\n");
#endif
}

__attribute__((section(".preinit_array")))
void (*dune_autoenter_preinit_entry)(void) = &dune_autoenter;
