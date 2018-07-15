#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/assert.h"
#include "dangless/common/statistics.h"
#include "dangless/common/dprintf.h"
#include "dangless/common/util.h"
#include "dangless/syscallmeta.h"
#include "dangless/dangless_malloc.h"

#include "vmcall_prehook.h"
#include "vmcall_fixup.h"

#if DANGLESS_CONFIG_DEBUG_DUNE_VMCALL_PREHOOK
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

STATISTIC_DEFINE_COUNTER(st_vmcall_count);

static THREAD_LOCAL int g_vmcall_hook_depth = 0;

bool is_vmcall_hook_running(void) {
  return g_vmcall_hook_depth > 0;
}

bool in_internal_vmcall(void) {
  return dangless_is_hook_running();
}

static void syscall_log_trace(u64 syscallno, u64 args[]) {
  // Usually we'll want to ignore write() with fd = stdout or stderr because there tend to be many of them during debugging that make it difficult to spot the interesting output.

  #if DANGLESS_CONFIG_TRACE_SYSCALLS_NO_WRITE_STDOUT
    if (syscallno == 1 && args[0] == 1)
      return;
  #endif

  #if DANGLESS_CONFIG_TRACE_SYSCALLS_NO_WRITE_STDERR
    if (syscallno == 1 && args[0] == 2)
      return;
  #endif

  syscall_log(syscallno, args);
}

void dangless_vmcall_prehook(u64 syscallno, REF u64 args[]) {
  ASSERT(g_vmcall_hook_depth == 0, "Nested vmcall prehook calls?");
  g_vmcall_hook_depth++;

  if (in_external_vmcall()) {
    #if DANGLESS_CONFIG_TRACE_SYSCALLS
      syscall_log_trace(syscallno, args);
    #endif

    STATISTIC_UPDATE() {
      st_vmcall_count++;
    }

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
        syscall_log(syscall, args); \
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

  g_vmcall_hook_depth--;
}
