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

STATISTIC_DEFINE_COUNTER(st_vmcall_count);
STATISTIC_DEFINE_COUNTER(st_vmcall_ptrarg_rewrites);
STATISTIC_DEFINE_COUNTER(st_vmcall_ptrarg_rewrite_misses);

STATISTIC_DEFINE_COUNTER(st_init_happened);

static void syscall_log(u64 syscallno, u64 args[]) {
  #if DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO
    const struct syscall_info *info = syscall_get_info(syscallno);

    dprintf("syscall: %s(\n", info->name);
    for (index_t i = 0; i < info->num_params; i++) {
      dprintf("\t%s %s = %lx\n", info->params[i].type, info->params[i].name, args[i]);
    }
    dprintf(")\n");
  #else
    dprintf("syscall: %1$lu (0x%1$lx) (\n", syscallno);
    for (index_t i = 0; i < SYSCALL_MAX_ARGS; i++) {
      dprintf("\targ%1$ld = %2$lu (0x%2$lx)\n", i, args[i]);
    }
    dprintf(")\n");
  #endif
}

static void process_syscall_ptr_arg(u64 syscall, REF u64 args[], index_t ptr_arg_index) {
  ASSERT(ptr_arg_index < SYSCALL_MAX_ARGS, "System call argument index %lu out of range!", ptr_arg_index);

  const u64 arg_ptr_raw = args[ptr_arg_index];
  void *const arg_ptr = (void *)arg_ptr_raw;

  // filter out typical null pointer, and often also just garbage for syscall args that are ignored (because e.g. they would only be used with certain flags)
  if (arg_ptr_raw < PGSIZE)
    return;

  STATISTIC_UPDATE() {
    st_vmcall_ptrarg_rewrites++;
  }

  void *canonical_ptr;
  int result = vremap_resolve(arg_ptr, OUT &canonical_ptr);

  if (result == VREM_OK) {
    //LOG("%p => canonical %p\n", arg_ptr, canonical_ptr);

    REF args[ptr_arg_index] = (u64)canonical_ptr;
  } else if (result != VREM_NOT_REMAPPED) {
    STATISTIC_UPDATE() {
      st_vmcall_ptrarg_rewrite_misses++;
    }

    syscall_log(syscall, args);

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
  syscall_log(syscall, args);
#endif

  STATISTIC_UPDATE() {
    st_vmcall_count++;
  }

  // some system calls are particularly interesting, such as fork() and execve(), log them separately
  switch (syscall) {
#define _INTERESTING_VMCALL_IMPL(NO, NAME) \
    LOG("INTERESTING VMCALL: " #NAME " (" STRINGIFY(NO) ")!\n"); \
    break

#if DANGLESS_CONFIG_TRACE_SYSCALLS
  #define INTERESTING_VMCALL(NO, NAME) \
    case NO: \
      _INTERESTING_VMCALL_IMPL(NO, NAME)
#else
  #define INTERESTING_VMCALL(NO, NAME) \
    case NO: \
      syscall_log(syscall, args); \
      _INTERESTING_VMCALL_IMPL(NO, NAME);
#endif

  INTERESTING_VMCALL(56, clone);
  INTERESTING_VMCALL(57, fork);
  INTERESTING_VMCALL(58, vfork);
  INTERESTING_VMCALL(59, execve);

#undef _INTERESTING_VMCALL_IMPL
#undef INTERESTING_VMCALL
  }

  // Rewrite virtually remapped pointer arguments to their canonical variants, as the host kernel cannot possibly know anything about things that only exists in the guest's virtual memory, therefore those user pointers would appear invalid for it.
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
  dune_die(); // RIP
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
  void (*dangless_preinit_entry)(void) = &dangless_init;
#endif
