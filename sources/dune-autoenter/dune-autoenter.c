#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "dune.h"

static void dune_autoenter(void) {
  int result;
  if ((result = dune_init(/*map_full=*/true)) != 0) {
    perror("Dune-autoenter: failed to initialize Dune");
    abort();
  }

  if ((result = dune_enter()) != 0) {
    perror("Dune-autoenter: failed to enter Dune mode");
    abort();
  }

  //fprintf(stderr, "Dune-autoenter: now in Dune mode!\n");
}

__attribute__((section(".preinit_array")))
static void (*preinit_entry)(void) = &dune_autoenter;
