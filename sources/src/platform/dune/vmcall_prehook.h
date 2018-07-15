#ifndef DANGLESS_DUNE_VMCALL_PREHOOK_H
#define DANGLESS_DUNE_VMCALL_PREHOOK_H

#include "dangless/common/types.h"

// Determines whether we're currently executing a vmcall hook (on this thread).
bool is_vmcall_hook_running(void);

// Determines whether the vmcall currently intercepted was initiated internally.
bool in_internal_vmcall(void);

// Determines whether the vmcall currently intercepted was initiated externally, i.e. not inside dangless.
static inline bool in_external_vmcall(void) {
  return !in_internal_vmcall();
}

void dangless_vmcall_prehook(u64 syscall, REF u64 args[]);

#endif
