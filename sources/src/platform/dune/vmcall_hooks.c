#include <string.h> // memcpy
#include <errno.h> // errno

#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/assert.h"
#include "dangless/common/statistics.h"
#include "dangless/common/dprintf.h"
#include "dangless/common/util.h"
#include "dangless/syscallmeta.h"
#include "dangless/dangless_malloc.h"

#include "dune.h"
#include "vmcall_hooks.h"
#include "vmcall_fixup.h"
#include "vmcall_fixup_restore.h"
#include "vmcall_handle_fork.h"

#if DANGLESS_CONFIG_DEBUG_DUNE_VMCALL_PREHOOK
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

STATISTIC_DEFINE_COUNTER(st_vmcall_count);

STATISTIC_DEFINE_COUNTER(st_internal_mmap_total);
STATISTIC_DEFINE_COUNTER(st_external_mmap_total);
STATISTIC_DEFINE_COUNTER(st_brk_total);

static THREAD_LOCAL int g_vmcall_hook_depth = 0;

u64 g_current_syscallno;
u64 g_current_syscall_args[SYSCALL_MAX_ARGS];
u64 g_current_syscall_return_addr;

void vmcall_hooks_init(void) {
  // Functions to run before and after system calls originating in ring 0 are passed on to the host kernel. Defined in vmcall_hooks.c.
  // Does not run when a vmcall is initiated manually, e.g. by dune_passthrough_syscall().
  __dune_vmcall_prehook = &dangless_vmcall_prehook;
  __dune_vmcall_posthook = &dangless_vmcall_posthook;
}

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

  // NOTE: for these comparisons, ALWAYS cast integer literals explicitly to u64, otherwise the check will silently get optimized out by GCC. WTF!!

  // ignore gettid()
  if (g_current_syscallno == (u64)186)
    return false;

  // Usually we'll want to ignore write() with fd = stdout or stderr because there tend to be many of them during debugging that make it difficult to spot the interesting output.

  #if DANGLESS_CONFIG_TRACE_SYSCALLS_NO_WRITE_STDOUT
    if (g_current_syscallno == (u64)1 && g_current_syscall_args[0] == (u64)1)
      return false;
  #endif

  #if DANGLESS_CONFIG_TRACE_SYSCALLS_NO_WRITE_STDERR
    if (g_current_syscallno == (u64)1 && g_current_syscall_args[0] == (u64)2)
      return false;
  #endif

  return true;
}

void vmcall_dump_current(void) {
  ASSERT0(is_vmcall_hook_running());
  syscall_log(g_current_syscallno, g_current_syscall_args);
}

static void update_mem_stats(void) {
  switch (g_current_syscallno) {
  case (u64)9: { // mmap()
    const size_t mmap_len = (size_t)g_current_syscall_args[1];
    if (in_external_vmcall()) {
      st_external_mmap_total += mmap_len;
    } else {
      st_internal_mmap_total += mmap_len;
    }
    break;
  }

  case (u64)12: { // brk()
    static u64 s_last_brk;

    const u64 brk_ptr = g_current_syscall_args[0];
    if (brk_ptr > s_last_brk) {
      if (s_last_brk) {
        st_brk_total += brk_ptr - s_last_brk;
      }

      s_last_brk = brk_ptr;
    }
    break;
  }
  } // end switch
}

void dangless_vmcall_prehook(REF u64 *psyscallno, REF u64 args[], REF u64 *pretaddr) {
  ASSERT(g_vmcall_hook_depth == 0, "Nested vmcall hook calls?");
  g_vmcall_hook_depth++;

  u64 syscallno = g_current_syscallno = *psyscallno;
  memcpy(g_current_syscall_args, args, sizeof(args[0]) * SYSCALL_MAX_ARGS);
  g_current_syscall_return_addr = *pretaddr;

  STATISTIC_UPDATE() {
    if (in_external_vmcall())
      st_vmcall_count++;

    update_mem_stats();
  }

  if (vmcall_should_trace_current()) {
    #if DANGLESS_CONFIG_TRACE_SYSCALLS
      vmcall_dump_current();
    #endif

    // some system calls are particularly interesting, such as fork() and execve(), log them separately
    /*switch (syscallno) {
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
    }*/
  }

  // The host kernel will choke on memory addresses that have been remapped by dangless, since those memory regions are only mapped inside the guest virtual memory, but do not appear in the host virtual memory. Therefore, for the vmcalls, we have to detect such memory addresses referenced directly or indirectly (e.g. through a struct) by their arguments and replace them with their canonical counterparts.
  vmcall_fixup_args(syscallno, REF args);

  // handle clone(), fork() and vfork() respectively
  // TODO: this is currently broken and disabled
  /*if (syscallno == (u64)56
      || syscallno == (u64)57
      || syscallno == (u64)58) {
    vmcall_handle_fork(REF pretaddr);
  }*/

  if (vmcall_should_trace_current()) {
    LOG("VMCall pre-hook returning...\n");
  }

  g_vmcall_hook_depth--;
}

void dangless_vmcall_posthook(REF u64 *presult) {
  ASSERT(g_vmcall_hook_depth == 0, "Nested vmcall hook calls?");
  g_vmcall_hook_depth++;

  const u64 result = *presult;

  if (vmcall_should_trace_current()) {
    LOG("VMCall post-hook running with result: 0x%lx (%lu)...\n", result, result);
  }

  // Hotfix for handling the issue with brk().
  // Dune by default only identity-maps the first 4 GB of memory.
  // The default malloc() implementation uses brk() to get more memory for small-ish allocations (tested with 9000 bytes).
  // If too many small allocations like this occur, at some point brk() will pass the 4 GB mark, and enter into a virtual memory region that's not mapped.
  // What we do here is, upon detecting that happening, we return 0 from the syscall, pretending it failed. The glibc malloc() implementation will then fall back to using mmap().
  // It's a cheap solution, and not ideal, but Good Enough (TM) for now.

  // TODO: this should be done in the prehook, and the vmcall not performed
  if (g_current_syscallno == (u64)12 && g_current_syscall_args[0] > (u64)0x100000000uL) {
    LOG("brk() call above 4 GB, returning 0 and setting errno (had result: 0x%lx)!\n", result);

    // we're supposed to return the old program break point on failure, but returning 0 seems to be good enough for glibc
    REF *presult = (u64)0;
    errno = ENOMEM;
  }

  vmcall_fixup_restore_originals();

  g_vmcall_hook_depth--;
}
