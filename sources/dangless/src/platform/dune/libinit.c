#include <stdlib.h>

#include "common.h"
#include "config.h"

#include "dune.h"

static void lib_init(void) {
  static bool s_initalized = false;
  if (s_initalized)
    return;

  s_initalized = true;

  dprintf("Entering Dune...\n");

  int result;
  if ((result = dune_init_and_enter()) != 0) {
    // TODO: a more elegant solution to this
    dprintf("Failed to enter Dune mode: result %d!\n", result);
    abort();
  }

  dprintf("Dune entered successfully. Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  void (*preinit_entry)(void) = &lib_init;
#endif
