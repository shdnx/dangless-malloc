#include <sys/uio.h> // for struct iovec
#include <sys/socket.h> // for struct msghdr

#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/assert.h"
#include "dangless/common/dprintf.h"
#include "dangless/common/statistics.h"
#include "dangless/common/util.h"
#include "dangless/common/debug.h"
#include "dangless/syscallmeta.h"
#include "dangless/platform/virtual_remap.h"

#include "vmcall_hooks.h"
#include "vmcall_fixup_restore.h"
#include "vmcall_fixup_info.h"
#include "vmcall_fixup.h"

// stack_base, mmap_base, etc.
#include "dune.h"

#if DANGLESS_CONFIG_DEBUG_DUNE_VMCALL_FIXUP
  #define LOG(...) \
    if (vmcall_should_trace_current()) \
      vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

STATISTIC_DEFINE_COUNTER(st_vmcall_arg_needed_fixups);
STATISTIC_DEFINE_COUNTER(st_vmcall_ptr_fixups);
STATISTIC_DEFINE_COUNTER(st_vmcall_ptr_fixup_misses);
STATISTIC_DEFINE_COUNTER(st_vmcall_ptr_fixup_failures);
STATISTIC_DEFINE_COUNTER(st_vmcall_nested_ptr_fixups);
STATISTIC_DEFINE_COUNTER(st_vmcall_invalid_ptrs);

// NOTE: we have to make sure that we don't crash if we encounter an invalid pointer, we just have to leave those alone.

static THREAD_LOCAL int g_fixup_depth = 0;

bool vmcall_fixup_is_running(void) { return g_fixup_depth > 0; }

static bool is_running_nested_fixup(void) { return g_fixup_depth > 1; }
static void fixup_enter(void *ptr) { g_fixup_depth++; }

static void fixup_leave(void *ptr) {
  ASSERT(g_fixup_depth > 0, "Unmatched fixup_enter() and fixup_leave()? Depth is at %d", g_fixup_depth);
  g_fixup_depth--;
}

#define FIXUP_SCOPE(PTR) CODE_CONTEXT(fixup_enter((PTR)), fixup_leave((PTR)))

static bool is_trivially_not_remapped(void *ptr) {
  uintptr_t addr = (uintptr_t)ptr;

  // TODO: .text, .data, .rodata, .bss

  return
    // the stack is trivially never remapped by Dangless
    // this is based on dune_va_to_pa() in dune.h
    (stack_base <= addr && addr < stack_base + GPA_STACK_SIZE);
}

static int fixup_ptr(REF void **pptr, OUT bool *is_valid) {
  ASSERT0(pptr);
  void *original_ptr = *pptr;

  // fast-path for NULL and obviously invalid pointers (often generated through e.g. member accesses through NULL pointers)
  if ((uintptr_t)original_ptr < PGSIZE) {
    if (is_valid)
      OUT *is_valid = false;

    return VREM_NOT_REMAPPED;
  }

  LOG("Fixing up ptr %p...\n", original_ptr);

  if (is_trivially_not_remapped(original_ptr)) {
    LOG("Trivially not remapped, skipping\n");

    if (is_valid)
      OUT *is_valid = true;

    return VREM_NOT_REMAPPED;
  }

  void *canonical_ptr;
  int result = vremap_resolve(original_ptr, OUT &canonical_ptr);

  LOG("vremap_resolve => %p; %s\n", canonical_ptr, vremap_diag(result));

  if (result == VREM_OK) {
    REF *pptr = canonical_ptr;

    STATISTIC_UPDATE() {
      if (in_external_vmcall())
        st_vmcall_ptr_fixups++;
    }

    if (is_running_nested_fixup()) {
      // if this is a nested pointer fixup, then we'll need to restore it before returning from the system call, so that dangless_free() invalidates its remapped region
      vmcall_fixup_restore_on_return(pptr, original_ptr);

      STATISTIC_UPDATE() {
        if (in_external_vmcall())
          st_vmcall_nested_ptr_fixups++;
      }
    }
  } else if (result == VREM_NOT_REMAPPED) {
    STATISTIC_UPDATE() {
      if (in_external_vmcall())
        st_vmcall_ptr_fixup_misses++;
    }
  } else {
    LOG("Failed to fix-up %p: error %s (code %d)\n", original_ptr, vremap_diag(result), result);

    STATISTIC_UPDATE() {
      if (in_external_vmcall()) {
        if (result == EVREM_NO_PHYS_BACK)
          st_vmcall_invalid_ptrs++;
        else
          st_vmcall_ptr_fixup_failures++;
      }
    }
  }

  if (is_valid)
    OUT *is_valid = (result != EVREM_NO_PHYS_BACK);

  return result;
}

// execve(), execveat() and friends
static int fixup_ptr_ptr_nullterm(REF void ***pptrs) {
  int result = 0;

  bool is_valid_ptr;
  if (fixup_ptr(REF (void **)pptrs, OUT &is_valid_ptr) < 0)
    result = -2;

  if (!is_valid_ptr)
    return -1;

  void **ptrs = *pptrs;

  if (vmcall_should_trace_current()) {
    LOG("Fixing up null-terminated ptr array of...\n");

    size_t num_items;
    for (num_items = 0; ptrs[num_items]; num_items++)
      /*empty*/;

    LOG("... %zu items\n", num_items);
  }

  FIXUP_SCOPE(pptrs) {
    for (size_t i = 0; ptrs[i]; i++) {
      LOG("Fixing up item %zu...\n", i);
      if (fixup_ptr(REF &ptrs[i], OUT_IGNORE) < 0) {
        LOG("Failed to fix-up item %zu in null-terminated pointer array %p\n", i, (void *)ptrs);
        result--;
      }
    }
  }

  LOG("Done, result: %d\n", result);
  return result;
}

/*
struct iovec {
  void  *iov_base;
  size_t iov_len;
};
*/

// e.g. writev(), readv() and some socket syscalls
static int fixup_iovec_array(REF struct iovec ARRAY **piovecs, size_t num_iovecs) {
  int result = 0;

  bool is_valid_ptr;
  if (fixup_ptr(REF (void **)piovecs, OUT &is_valid_ptr) < 0)
    result = -2;

  if (!is_valid_ptr)
    return -1;

  LOG("Fixing up iovec array of %zu items:\n", num_iovecs);

  #if DANGLESS_CONFIG_DEBUG_DUNE_VMCALL_FIXUP
    dump_mem_region(*piovecs, num_iovecs * sizeof(struct iovec));
  #endif

  FIXUP_SCOPE(piovecs) {
    for (size_t i = 0; i < num_iovecs; i++) {
      LOG("Fixing up item %zu...\n", i);
      if (fixup_ptr(REF &(*piovecs)[i].iov_base, OUT_IGNORE) < 0) {
        LOG("Failed to fix-up item %zu of iovec array %p\n", i, (void *)*piovecs);
        result--;
      }
    }
  }

  LOG("Done, result: %d\n", result);
  return result;
}

static int fixup_msghdr(REF struct msghdr **pmsghdr) {
  /*
  struct msghdr {
    void         *msg_name;
    socklen_t     msg_namelen;
    struct iovec *msg_iov;
    size_t        msg_iovlen;
    void         *msg_control;
    size_t        msg_controllen;
    int           msg_flags;
  };
  */

  int result = 0;

  bool is_valid_ptr;
  if (fixup_ptr(REF (void **)pmsghdr, OUT &is_valid_ptr) < 0)
    result = -2;

  if (!is_valid_ptr)
    return -1;

  struct msghdr *msghdr = *pmsghdr;

  LOG("Fixing up msghdr:\n");
  #if DANGLESS_CONFIG_DEBUG_DUNE_VMCALL_FIXUP
    DUMP_VAR_MEM(*msghdr);
  #endif

  FIXUP_SCOPE(pmsghdr) {
    LOG("msg_name...\n");

    if (fixup_ptr(REF (void **)&msghdr->msg_name, OUT_IGNORE) < 0) {
      LOG("Failed to fix-up msg_name of msghdr %p\n", (void *)msghdr);
      result--;
    }

    LOG("msg_iov...\n");

    if (fixup_iovec_array(REF &msghdr->msg_iov, msghdr->msg_iovlen) < 0) {
      LOG("Failed to fix-up msg_iov of msghdr %p\n", (void *)msghdr);
      result--;
    }

    LOG("msg_control...\n");

    if (fixup_ptr(REF &msghdr->msg_control, OUT_IGNORE) < 0) {
      LOG("Failed to fix-up msg_control of msghdr %p\n", (void *)msghdr);
      result--;
    }
  }

  LOG("Done, result: %d\n", result);
  return result;
}

// Rewrite virtually remapped pointer arguments to their canonical variants, as the host kernel cannot possibly know anything about things that only exists in the guest's virtual memory, therefore those user pointers would appear invalid for it.
// Note that pointers can also be nested, e.g. in arrays and even inside various structs.
int vmcall_fixup_args(u64 syscallno, u64 args[]) {
  const struct vmcall_fixup_info *fixup_info = vmcall_get_fixup_info(syscallno);

  if (!fixup_info) {
    LOG("cannot fixup vmcall arguments for system call %lu - no fixup info available", syscallno);
    return 0;
  }

  int final_result = 0;

  for (size_t arg_index = 0; arg_index < SYSCALL_MAX_ARGS; ++arg_index) {
    const enum vmcall_param_fixup_type arg_fixup_type = fixup_info->params[arg_index].fixup_type;

    if (arg_fixup_type == VMCALL_PARAM_END)
      break;

    if (arg_fixup_type == VMCALL_PARAM_NONE
        || args[arg_index] < PGSIZE /*trivially invalid pointer*/)
      continue;

    STATISTIC_UPDATE() {
      if (in_external_vmcall())
        st_vmcall_arg_needed_fixups++;
    }

    LOG("Fixup of arg %zu = %lx with fixup type: %s\n", arg_index, args[arg_index], vmcall_param_fixup_type_str(arg_fixup_type));

    u64 *parg = &args[arg_index];
    int result;

    FIXUP_SCOPE(parg) {
      switch (arg_fixup_type) {
      case VMCALL_PARAM_FLAT_PTR:
        result = fixup_ptr(REF (void **)parg, OUT_IGNORE);
        break;

      case VMCALL_PARAM_IOVEC: {
        // the length of the iovec array seems to be always passed in as the next argument
        arg_index++;
        const size_t num_iovecs = (size_t)args[arg_index];
        result = fixup_iovec_array(REF (struct iovec **)parg, num_iovecs);
        break;
      }

      case VMCALL_PARAM_PTR_PTR:
        result = fixup_ptr_ptr_nullterm(REF (void ***)parg);
        break;

      case VMCALL_PARAM_MSGHDR:
        result = fixup_msghdr(REF (struct msghdr **)parg);
        break;

      case VMCALL_PARAM_NONE:
      case VMCALL_PARAM_END:
        UNREACHABLE("Unreachable fixup type - should have already been handled");
      }
    }

    LOG("Done fixup of arg %zu\n", arg_index);

    if (result < 0) {
      final_result--;

      #if DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO
        const struct syscall_info *sinfo = syscall_get_info(syscallno);

        LOG("Failed to fixup syscall %s arg %zu (%s %s): ",
          sinfo->name,
          arg_index,
          sinfo->params[arg_index].name,
          sinfo->params[arg_index].type);
      #else
        LOG("Failed to fixup syscall %lu arg %zu: ", syscallno, arg_index);
      #endif

      syscall_log(syscallno, args);

      // note: we specifically don't want to stop if we encounter a failure, we'll just try to fixup everything we can
    }
  } // end for

  return final_result;
}
