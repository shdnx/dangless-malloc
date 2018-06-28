#define _GNU_SOURCE

#include <stdlib.h>
#include <stdio.h> // perror

#include "dangless/config.h"
#include "dangless/common.h"
#include "dangless/common/statistics.h"
#include "dangless/syscallmeta.h"
#include "dangless/virtmem.h"
#include "dangless/platform/virtual_remap.h"

#include "dune.h"

#if DANGLESS_CONFIG_DEBUG_INIT
  #define LOG(...) vdprintf(__VA_ARGS__)
  #define LOG_NOMALLOC(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
  #define LOG_NOMALLOC(...) /* empty */
#endif

STATISTIC_DEFINE(size_t, vmcall_count);
STATISTIC_DEFINE(size_t, vmcall_ptrarg_rewrites);
STATISTIC_DEFINE(size_t, vmcall_ptrarg_rewrite_misses);

static void process_syscall_ptr_arg(u64 syscall, REF u64 args[], index_t ptr_arg_index) {
  ASSERT(ptr_arg_index < SYSCALL_MAX_ARGS, "System call argument index %lu out of range!", ptr_arg_index);

  void *arg_ptr = (void *)args[ptr_arg_index];

  if (!arg_ptr)
    return; // null pointer

  STATISTIC_UPDATE() {
    vmcall_ptrarg_rewrites++;
  }

  void *canonical_ptr;
  int result = vremap_resolve(arg_ptr, OUT &canonical_ptr);

  if (result == VREM_OK) {
    //LOG("%p => canonical %p\n", arg_ptr, canonical_ptr);

    REF args[ptr_arg_index] = (u64)canonical_ptr;
  } else if (result != VREM_NOT_REMAPPED) {
    STATISTIC_UPDATE() {
      vmcall_ptrarg_rewrite_misses++;
    }

    #if DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO
      const struct syscall_info *sinfo = syscall_get_info(syscall);

      LOG("syscall %s arg %lu (%s %s): ",
        sinfo->name,
        ptr_arg_index,
        sinfo->params[ptr_arg_index].name,
        sinfo->params[ptr_arg_index].type);
    #else
      LOG("syscall %lu arg %lu: ", syscall, ptr_arg_index);
    #endif

    LOG("could not translate %p: %s (code %d)\n", arg_ptr, vremap_diag(result), result);
  }
}

static void dangless_vmcall_prehook(u64 syscall, REF u64 args[]) {
#if DANGLESS_CONFIG_TRACE_SYSCALLS
  #if DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO
    const struct syscall_info *info = syscall_get_info(syscall);

    vdprintf("syscall: %s(\n", info->name);
    for (index_t i = 0; i < info->num_params; i++) {
      dprintf("\t%s %s = %lx\n", info->params[i].type, info->params[i].name, args[i]);
    }
    dprintf(")\n");
  #else
    vdprintf("syscall: %1$lu (0x%1$lx) (\n", syscall);
    for (index_t i = 0; i < SYSCALL_MAX_ARGS; i++) {
      dprintf("\targ%1$ld = %2$lu (0x%2$lx)\n", i, args[i]);
    }
    dprintf(")\n");
  #endif
#endif

  STATISTIC_UPDATE() {
    vmcall_count++;
  }

  for (const i8 *ptrparam_no = syscall_get_userptr_params(syscall);
       ptrparam_no && *ptrparam_no > 0;
       ptrparam_no++) {
    process_syscall_ptr_arg(syscall, REF args, (index_t)*ptrparam_no - 1);
  }
}

enum {
  PGFLT_FLAG_ISPRESENT  = 0x1,
  PGFLT_FLAG_ISWRITE    = 0x2,
  PGFLT_FLAG_ISUSER     = 0x4
};

static void dangless_pagefault_handler(vaddr_t addr_, u64 flags, struct dune_tf *tf) {
  void *const addr = (void *)addr_;
  void *const addr_page = (void *)ROUND_DOWN(addr_, PGSIZE);

  // dune_printf() is guaranteed to work, whereas my code is not :P
  dune_printf(
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
      dune_printf("NOTE: Access would have been through a dangling pointer: PTE %p invalidated on level %u\n", ppte, level);
    }
  }

  dune_printf("\n");
  dune_dump_trap_frame(tf);
  dune_die(); // RIP
}

void dangless_init(void) {
  static bool s_initalized = false;
  if (s_initalized)
    return;

  s_initalized = true;

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

  // Function to run before system calls originating in ring 0 are passed on to the host kernel.
  // Does not run when a vmcall is initiated manually, e.g. by dune_passthrough_syscall().
  __dune_vmcall_prehook = &dangless_vmcall_prehook;

  // Register page-fault handler, for diagnostic purposes
  dune_register_pgflt_handler(&dangless_pagefault_handler);

  LOG("Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  void (*preinit_entry)(void) = &dangless_init;
#endif
