#define _GNU_SOURCE

#include <stdlib.h>

#include <errno.h>

#include "config.h"
#include "common.h"

#include "dune.h"

#if INIT_DEBUG
  #define LOG(...) vdprintf(__VA_ARGS__)
  #define LOG_NOMALLOC(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
  #define LOG_NOMALLOC(...) /* empty */
#endif

void dangless_init(void) {
  static bool s_initalized = false;
  if (s_initalized)
    return;

  s_initalized = true;

  LOG_NOMALLOC("Entering Dune...\n");

  int result;
  if ((result = dune_init_and_enter()) != 0) {
    perror("Dangless: failed to enter Dune mode");
    abort();
  }

  LOG_NOMALLOC("Dune entered successfully. Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  void (*preinit_entry)(void) = &dangless_init;
#endif
