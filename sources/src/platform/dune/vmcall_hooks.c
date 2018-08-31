#include <string.h> // memcpy

#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/assert.h"
#include "dangless/common/statistics.h"
#include "dangless/common/dprintf.h"
#include "dangless/common/util.h"
#include "dangless/syscallmeta.h"
#include "dangless/dangless_malloc.h"

#include "vmcall_hooks.h"
#include "vmcall_fixup.h"

#if DANGLESS_CONFIG_DEBUG_DUNE_VMCALL_PREHOOK
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

STATISTIC_DEFINE_COUNTER(st_vmcall_count);

static THREAD_LOCAL int g_vmcall_hook_depth = 0;

u64 g_current_syscallno;
u64 g_current_syscall_args[SYSCALL_MAX_ARGS];

bool is_vmcall_hook_running(void) {
  return g_vmcall_hook_depth > 0;
}

bool in_internal_vmcall(void) {
  ASSERT0(is_vmcall_hook_running());

  return dangless_is_hook_running();
}

bool vmcall_should_trace_current(void) {
  ASSERT0(is_vmcall_hook_running());

  if (in_internal_vmcall())
    return false;

  // ignore gettid()
  if (g_current_syscallno == 186)
    return false;

  // Usually we'll want to ignore write() with fd = stdout or stderr because there tend to be many of them during debugging that make it difficult to spot the interesting output.

  #if DANGLESS_CONFIG_TRACE_SYSCALLS_NO_WRITE_STDOUT
    if (g_current_syscallno == 1 && g_current_syscall_args[0] == 1)
      return false;
  #endif

  #if DANGLESS_CONFIG_TRACE_SYSCALLS_NO_WRITE_STDERR
    if (g_current_syscallno == 1 && g_current_syscall_args[0] == 2)
      return false;
  #endif

  return true;
}

void vmcall_dump_current(void) {
  ASSERT0(is_vmcall_hook_running());
  syscall_log(g_current_syscallno, g_current_syscall_args);
}

void dangless_vmcall_prehook(u64 syscallno, REF u64 args[]) {
  ASSERT(g_vmcall_hook_depth == 0, "Nested vmcall hook calls?");
  g_vmcall_hook_depth++;

  g_current_syscallno = syscallno;
  memcpy(g_current_syscall_args, args, sizeof(args[0]) * SYSCALL_MAX_ARGS);

  STATISTIC_UPDATE() {
    if (in_external_vmcall())
      st_vmcall_count++;
  }

  if (vmcall_should_trace_current()) {
    #if DANGLESS_CONFIG_TRACE_SYSCALLS
      vmcall_dump_current();
    #endif

    // some system calls are particularly interesting, such as fork() and execve(), log them separately
    switch (syscallno) {
  #define _INTERESTING_VMCALL_IMPL(NO, NAME) \
      LOG("MARK interesting vmcall: " #NAME " (" STRINGIFY(NO) ")!\n"); \
      break

  #if DANGLESS_CONFIG_TRACE_SYSCALLS
    #define INTERESTING_VMCALL(NO, NAME) \
      case NO: \
        _INTERESTING_VMCALL_IMPL(NO, NAME)
  #else
    #define INTERESTING_VMCALL(NO, NAME) \
      case NO: \
        syscall_log(syscallno, args); \
        _INTERESTING_VMCALL_IMPL(NO, NAME)
  #endif

    INTERESTING_VMCALL(56, clone);
    INTERESTING_VMCALL(57, fork);
    INTERESTING_VMCALL(58, vfork);
    INTERESTING_VMCALL(59, execve);

  #undef _INTERESTING_VMCALL_IMPL
  #undef INTERESTING_VMCALL
    }
  }

  // The host kernel will choke on memory addresses that have been remapped by dangless, since those memory regions are only mapped inside the guest virtual memory, but do not appear in the host virtual memory. Therefore, for the vmcalls, we have to detect such memory addresses referenced directly or indirectly (e.g. through a struct) by their arguments and replace them with their canonical counterparts.
  vmcall_fixup_args(syscallno, REF args);

  if (vmcall_should_trace_current()) {
    LOG("VMCall pre-hook returning...\n");
  }

  g_vmcall_hook_depth--;
}

void dangless_vmcall_posthook(u64 result) {
  ASSERT(g_vmcall_hook_depth == 0, "Nested vmcall hook calls?");
  g_vmcall_hook_depth++;

  if (vmcall_should_trace_current()) {
    LOG("VMCall post-hook running with result: 0x%lx (%lu)...\n", result, result);
  }

  g_vmcall_hook_depth--;
}
