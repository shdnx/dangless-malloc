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

/*static inline u64 *syscall_no(struct dune_tf *tf) {
  return &tf->rax;
}

static inline u64 *syscall_arg(struct dune_tf *tf, index_t argnum) {
  ASSERT(argnum < SYSCALL_MAX_NUM_ARGS, "System calls cannot have more than %d arguments!", SYSCALL_MAX_NUM_ARGS);

  // since in struct dune_tf, the fields are in the same order as syscall argument registers, we can just pretend that dune_tf is an array of u64-s
  return &tf->rdi + argnum;
}*/

static void syscall_process_ptr_arg(u64 syscall, REF u64 *args, index_t ptr_arg_index) {
  ASSERT(ptr_arg_index < SYSCALL_MAX_NUM_ARGS, "System call argument index %lu out of range!", ptr_arg_index);

  void *arg_ptr = (void *)args[ptr_arg_index];

  if (!arg_ptr)
    return; // null pointer

  void *canonical_ptr;
  int result = vremap_resolve(arg_ptr, OUT &canonical_ptr);

  if (result == VREM_OK) {
    LOG("%p => canonical %p\n", arg_ptr, canonical_ptr);

    REF args[ptr_arg_index] = (u64)canonical_ptr;
  } else {
    LOG("syscall %s arg %lu (%s): ", syscall_get_name(syscall), ptr_arg_index, syscall_get_info(syscall)->param_types[ptr_arg_index]);
    LOG("could not translate %p: code %d\n", arg_ptr, result);
  }
}

static void syscall_prehook(u64 syscall, u64 *args) {
  //LOG("syscall %s\n", syscall_get_name(syscall));

  for (const int *ptrparam_no = syscall_get_userptr_params(syscall);
       ptrparam_no && *ptrparam_no > 0;
       ptrparam_no++) {
    syscall_process_ptr_arg(syscall, REF args, *ptrparam_no - 1);
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

  // Function to run before system calls originating in ring 0 are passed on to the host kernel. Defined in libdune/dune.S.
  extern void (*__dune_ring0_syscall_prehook)(u64 syscall, u64 *args);
  __dune_ring0_syscall_prehook = &syscall_prehook;

  LOG("Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  void (*preinit_entry)(void) = &dangless_init;
#endif
