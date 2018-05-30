#define _GNU_SOURCE

#include <stdlib.h>

#include <errno.h>

#include "dangless/config.h"
#include "dangless/common.h"
#include "dangless/syscallmeta.h"
#include "dangless/platform/virtual_remap.h"

#include "dune.h"

#if DANGLESS_CONFIG_DEBUG_INIT
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

static inline u64 *syscall_no(struct dune_tf *tf) {
  return &tf->rax;
}

static inline u64 *syscall_arg(struct dune_tf *tf, index_t argnum) {
  ASSERT(argnum < SYSCALL_MAX_NUM_ARGS, "System calls cannot have more than %d arguments!", SYSCALL_MAX_NUM_ARGS);

  // since in struct dune_tf, the fields are in the same order as syscall argument registers, we can just pretend that dune_tf is an array of u64-s
  return &tf->rdi + argnum;
}

static void syscall_process_ptr_param(REF u64 *param) {
  void *param_ptr = (void *)*param;

  if (!*param)
    return; // null pointer

  void *canonical_ptr;
  int result = vremap_resolve(param_ptr, OUT &canonical_ptr);

  if (result == VREM_OK) {
    LOG("Translated remapped syscall pointer argument %p to canonical %p\n", param_ptr, canonical_ptr);

    REF *param = (u64)canonical_ptr;
  } else {
    LOG("Could not translate syscall pointer argument %p: code %d\n", param_ptr, result);
  }
}

// why doesn't this ever get executed???
static void syscall_handler(struct dune_tf *tf) {
  u64 syscallno = *syscall_no(tf);
  LOG("syscall %s (%lu)!\n", syscall_get_name(syscallno), syscallno);

  for (const int *ptr_param_no = syscall_get_userptr_params(syscallno);
       ptr_param_no && *ptr_param_no > 0;
       ptr_param_no++) {
    syscall_process_ptr_param(
      REF syscall_arg(tf, (index_t)(*ptr_param_no) - 1)
    );
  }

  dune_passthrough_syscall(tf);
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

  dune_register_syscall_handler(&syscall_handler);

  LOG("Running...\n");
}

#if DANGLESS_CONFIG_REGISTER_PREINIT
  __attribute__((section(".preinit_array")))
  void (*preinit_entry)(void) = &dangless_init;
#endif
