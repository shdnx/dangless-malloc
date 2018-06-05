#define _GNU_SOURCE

#include <stdlib.h>

#include <errno.h>

#include "dangless/config.h"
#include "dangless/common.h"
#include "dangless/syscallmeta.h"
#include "dangless/platform/virtual_remap.h"

#include "dune.h"

#if DANGLESS_CONFIG_DEBUG_INIT
  #define LOG(...) vdprintf(__VA_ARGS__)
  #define LOG_NOMALLOC(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
  #define LOG_NOMALLOC(...) /* empty */
#endif

static void process_syscall_ptr_arg(u64 syscall, REF u64 args[], index_t ptr_arg_index) {
  ASSERT(ptr_arg_index < SYSCALL_MAX_ARGS, "System call argument index %lu out of range!", ptr_arg_index);

  void *arg_ptr = (void *)args[ptr_arg_index];

  if (!arg_ptr)
    return; // null pointer

  void *canonical_ptr;
  int result = vremap_resolve(arg_ptr, OUT &canonical_ptr);

  if (result == VREM_OK) {
    //LOG("%p => canonical %p\n", arg_ptr, canonical_ptr);

    REF args[ptr_arg_index] = (u64)canonical_ptr;
  } else if (result != VREM_NOT_REMAPPED) {
    #if DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO
      LOG("syscall %s arg %lu (%s %s): ", syscall_get_name(syscall), ptr_arg_index, syscall_get_info(syscall)->params[ptr_arg_index].name, syscall_get_info(syscall)->params[ptr_arg_index].type);
    #else
      LOG("syscall %lu arg %lu: ", syscall, ptr_arg_index);
    #endif

    LOG("could not translate %p: %s (code %d)\n", arg_ptr, vremap_error(), result);
  }
}

static void dangless_vmcall_prehook(u64 syscall, REF u64 args[]) {
#if DANGLESS_CONFIG_TRACE_SYSCALLS
  #if DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO
    const struct syscall_info *info = syscall_get_info(syscall);
    vdprintf("syscall: %s(\n", info->name);
    for (index_t i = 0; i < info->num_params; i++) {
      vdprintf("\t%s %s = %lx\n", info->params[i].type, info->params[i].name, args[i]);
    }
    vdprintf(")\n");
  #else
    vdprintf("syscall: %1$lu (0x%1$lx) (\n", syscall);
    for (index_t i = 0; i < SYSCALL_MAX_ARGS; i++) {
      vdprintf("\targ%1$ld = %2$lu (0x%2$lx)\n", i, args[i]);
    }
    vdprintf(")\n");
  #endif
#endif

  for (const i8 *ptrparam_no = syscall_get_userptr_params(syscall);
       ptrparam_no && *ptrparam_no > 0;
       ptrparam_no++) {
    process_syscall_ptr_arg(syscall, REF args, (index_t)*ptrparam_no - 1);
  }
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

  LOG("Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  static void (*preinit_entry)(void) = &dangless_init;
#endif
