#define _GNU_SOURCE

#include <stdlib.h>

#include <errno.h>

#include "dangless/config.h"
#include "dangless/common.h"

#include "dune.h"

#if INIT_DEBUG
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

void dangless_init(void) {
  static bool s_initalized = false;
  if (s_initalized)
    return;

  s_initalized = true;

  LOG("Entering Dune...\n");

  int result;
  if ((result = dune_init_and_enter()) != 0) {
    perror("Dangless: failed to enter Dune mode");
    abort();
  }

  LOG("Dune entered successfully. Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  void (*preinit_entry)(void) = &dangless_init;
#endif
