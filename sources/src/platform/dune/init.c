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
  } else {
    /*LOG("syscall %s arg %lu (%s %s): ", syscall_get_name(syscall), ptr_arg_index, syscall_get_info(syscall)->params[ptr_arg_index].name, syscall_get_info(syscall)->params[ptr_arg_index].type);
    LOG("could not translate %p: code %d\n", arg_ptr, result);*/
  }
}

static void ring0_syscall_prehook(u64 syscall, REF u64 args[]) {
  /*const struct syscall_info *info = syscall_get_info(syscall);
  LOG("syscall: %s(\n", info->name);
  for (index_t i = 0; i < info->num_params; i++) {
    LOG("\t%s %s = %lx\n", info->params[i].type, info->params[i].name, args[i]);
  }
  LOG(")\n");*/

  for (const int *ptrparam_no = syscall_get_userptr_params(syscall);
       ptrparam_no && *ptrparam_no > 0;
       ptrparam_no++) {
    process_syscall_ptr_arg(syscall, REF args, *ptrparam_no - 1);
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

  __dune_ring0_syscall_prehook = &ring0_syscall_prehook;

  LOG("Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  void (*preinit_entry)(void) = &dangless_init;
#endif
