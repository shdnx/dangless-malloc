#ifndef DANGLESS_DUNE_VMCALL_HOOKS_H
#define DANGLESS_DUNE_VMCALL_HOOKS_H

#include "dangless/common/types.h"

// Determines whether we're currently executing a vmcall hook (on this thread).
bool is_vmcall_hook_running(void);

// Determines whether the vmcall currently intercepted was initiated internally.
bool in_internal_vmcall(void);

// Determines whether the vmcall currently intercepted was initiated externally, i.e. not inside dangless.
static inline bool in_external_vmcall(void) {
  return !in_internal_vmcall();
}

extern u64 g_current_syscallno;
extern u64 g_current_syscall_args[];

// Determines whether the current vmcall should be traced.
bool vmcall_should_trace_current(void);

// Dump's information about the current vmcall to stderr.
void vmcall_dump_current(void);

void dangless_vmcall_prehook(u64 syscall, REF u64 args[]);
void dangless_vmcall_posthook(u64 result);

#endif
