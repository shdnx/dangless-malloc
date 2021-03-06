#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h> // perror

#include "dangless/config.h"
#include "dangless/common.h"
#include "dangless/common/statistics.h"
#include "dangless/virtmem.h"

#include "vmcall_hooks.h"
#include "vmcall_fixup_restore.h"
#include "dune.h"

#if DANGLESS_CONFIG_DEBUG_INIT
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

STATISTIC_DEFINE_COUNTER(st_init_happened);

enum {
  PGFLT_FLAG_ISPRESENT  = 0x1,
  PGFLT_FLAG_ISWRITE    = 0x2,
  PGFLT_FLAG_ISUSER     = 0x4
};

// dune_printf() is guaranteed to work, whereas my code is not :P
#define PRINTF_SAFE(...) dune_printf(__VA_ARGS__)

static void dangless_pagefault_handler(vaddr_t addr_, u64 flags, struct dune_tf *tf) {
  void *const addr = (void *)addr_;
  void *const addr_page = (void *)ROUND_DOWN(addr_, PGSIZE);

  PRINTF_SAFE(
    "!!! Unhandled page fault on %p, flags: %lx [%s | %s | %s]\n",
    addr,
    flags,
    FLAG_ISSET(flags, PGFLT_FLAG_ISUSER) ? "user" : "kernel",
    FLAG_ISSET(flags, PGFLT_FLAG_ISWRITE) ? "write" : "read",
    FLAG_ISSET(flags, PGFLT_FLAG_ISPRESENT) ? "protection-violation" : "not-present"
  );

  // check whether this was a virtual address we invalidated, i.e. this would be a dangling pointer access
  if (!FLAG_ISSET(flags, PGFLT_FLAG_ISPRESENT)) {
    pte_t *ppte;
    enum pt_level level = pt_walk(addr_page, PGWALK_FULL, OUT &ppte);

    if (*ppte == PTE_INVALIDATED) {
      PRINTF_SAFE("NOTE: Access would have been through a dangling pointer: PTE %p invalidated on level %u\n", ppte, level);
    }
  }

  PRINTF_SAFE("\n");
  dune_dump_trap_frame(tf);

  // this seems to cause the application to hang endlessly, don't do it
  //dune_procmap_dump();

  dune_die();
}

void dangless_enter_dune(void) {
  LOG("Initializing Dune...\n");

  int result;
  if ((result = dune_init(/*map_full=*/true)) != 0) {
    perror("Dangless: failed to initialize Dune");
    abort();
  }

  LOG("Entering Dune...\n");

  if ((result = dune_enter()) != 0) {
    perror("Dangless: failed to enter Dune mode");
    abort();
  }

  LOG("Dune ready!\n");
}

void dangless_init(void) {
  STATISTIC_UPDATE() {
    st_init_happened++;
  }

  static bool s_initalized = false;
  if (s_initalized)
    return;

  s_initalized = true;

  LOG("Dangless init starting, platform = " STRINGIFY(DANGLESS_CONFIG_PLATFORM) ", profile = " STRINGIFY(DANGLESS_CONFIG_PROFILE) "\n");

  dangless_enter_dune();

  statistics_init();

  // allow fixed-up nested pointers to be restored to their original values when the vmcall returns
  vmcall_fixup_restore_init();

  vmcall_hooks_init();

  // for diagnostic purposes
  dune_register_pgflt_handler(&dangless_pagefault_handler);

  // unfortunately signal handling only works for userspace, it's useless for our purposes, otherwise it'd be nice to register a signal handler for SIGSEGV...

  LOG("Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  void (*dangless_preinit_entry)(void) = &dangless_init;
#endif
