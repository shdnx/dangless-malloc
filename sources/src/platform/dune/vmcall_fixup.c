#include <sys/syscall.h> // for the __NR_ macros
#include <sys/uio.h> // for struct iovec
#include <sys/socket.h> // for struct msghdr

#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/assert.h"
#include "dangless/common/dprintf.h"
#include "dangless/common/statistics.h"
#include "dangless/common/util.h"
#include "dangless/syscallmeta.h"
#include "dangless/platform/virtual_remap.h"

#include "vmcall_prehook.h"
#include "vmcall_fixup.h"

#if DANGLESS_CONFIG_DEBUG_DUNE_VMCALL_FIXUP
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
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

static THREAD_LOCAL bool g_is_running_nested_fixups = false;

static int fixup_ptr(REF void **pptr, OUT bool *is_valid) {
  ASSERT0(pptr);
  void *ptr = *pptr;

  if ((uintptr_t)ptr < PGSIZE) {
    if (is_valid)
      OUT *is_valid = false;

    return VREM_NOT_REMAPPED;
  }

  void *canonical_ptr;
  int result = vremap_resolve(ptr, OUT &canonical_ptr);

  if (result == VREM_OK) {
    REF *pptr = canonical_ptr;

    STATISTIC_UPDATE() {
      if (in_external_vmcall()) {
        st_vmcall_ptr_fixups++;

        if (g_is_running_nested_fixups)
          st_vmcall_nested_ptr_fixups++;
      }
    }
  } else if (result == VREM_NOT_REMAPPED) {
    STATISTIC_UPDATE() {
      if (in_external_vmcall())
        st_vmcall_ptr_fixup_misses++;
    }
  } else {
    LOG("Failed to fix-up %p: error %s (code %d)\n", ptr, vremap_diag(result), result);

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

  for (size_t i = 0; ptrs[i]; i++) {
    if (fixup_ptr(REF &ptrs[i], OUT_IGNORE) < 0) {
      LOG("Failed to fix-up item %zu in null-terminated pointer array %p\n", i, (void *)ptrs);
      result--;
    }
  }

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

  for (size_t i = 0; i < num_iovecs; i++) {
    if (fixup_ptr(REF &(*piovecs)[i].iov_base, OUT_IGNORE) < 0) {
      LOG("Failed to fix-up item %zu of iovec array %p\n", i, (void *)*piovecs);
      result--;
    }
  }

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

  if (fixup_ptr((void **)&msghdr->msg_name, OUT_IGNORE) < 0) {
    LOG("Failed to fix-up msg_name of msghdr %p\n", (void *)msghdr);
    result--;
  }

  if (fixup_iovec_array(REF &msghdr->msg_iov, msghdr->msg_iovlen) != 0) {
    LOG("Failed to fix-up msg_iov of msghdr %p\n", (void *)msghdr);
    result--;
  }

  if (fixup_ptr(msghdr->msg_control, OUT_IGNORE) < 0) {
    LOG("Failed to fix-up msg_control of msghdr %p\n", (void *)msghdr);
    result--;
  }

  return result;
}

enum syscall_param_fixup_flags {
  SYSCALL_PARAM_VALID     = 1 << 0,

  SYSCALL_PARAM_USER_PTR  = 1 << 1,
  SYSCALL_PARAM_IOVEC     = 1 << 2,
  SYSCALL_PARAM_PTR_PTR   = 1 << 3,
  SYSCALL_PARAM_MSGHDR    = 1 << 4,

  // indicates that the parameter is the last interesting one, no need to check the rest
  SYSCALL_PARAM_LAST      = 1 << 7
};

static void syscall_param_fixup_flags_dump(enum syscall_param_fixup_flags flags) {
  dprintf("%u (", flags);

  #define HANDLE_FLAG(FLAG) \
    do { \
      if (FLAG_ISSET(flags, CONCAT2(SYSCALL_PARAM_, FLAG))) \
        dprintf(#FLAG " "); \
    } while (0)

  HANDLE_FLAG(VALID);
  HANDLE_FLAG(USER_PTR);
  HANDLE_FLAG(IOVEC);
  HANDLE_FLAG(PTR_PTR);
  HANDLE_FLAG(MSGHDR);
  HANDLE_FLAG(LAST);

  dprintf(")");
}

static inline bool is_boring_syscall_param_flags(enum syscall_param_fixup_flags flags) {
  return flags == SYSCALL_PARAM_VALID
    || flags == (SYSCALL_PARAM_VALID | SYSCALL_PARAM_LAST);
}

static const u8 g_syscall_param_fixup_flags[][SYSCALL_MAX_ARGS] = {
  #include "dangless/build/common/syscall_param_fixup_flags.c"
};

int vmcall_fixup_args(u64 syscallno, u64 args[]) {
  // Rewrite virtually remapped pointer arguments to their canonical variants, as the host kernel cannot possibly know anything about things that only exists in the guest's virtual memory, therefore those user pointers would appear invalid for it.
  // Note that pointers can also be nested, e.g. in arrays and even inside various structs.

  int final_result = 0;

  const u8 *pparam_fixup_flags = g_syscall_param_fixup_flags[syscallno];
  for (size_t arg_index = 0;
       FLAG_ISSET(*pparam_fixup_flags, SYSCALL_PARAM_VALID);
       arg_index++, pparam_fixup_flags++
  ) {
    ASSERT0(arg_index < SYSCALL_MAX_ARGS);

    enum syscall_param_fixup_flags pfixup_flags = *pparam_fixup_flags;

    /*const struct syscall_info *info = syscall_get_info(syscallno);
    LOG("DBG: syscall %s (%lu) arg %zu (%s %s), fixup flags: ", info->name, syscallno, arg_index, info->params[arg_index].type, info->params[arg_index].name);
    syscall_param_fixup_flags_dump(pfixup_flags);
    //dprintf("%u", pfixup_flags);
    dprintf("\n");*/

    if (args[arg_index] < PGSIZE)
      goto loop_continue; // definitely not a valid pointer then

    // this check is an optimization: if it's a plain parameter, with no flags set other than VALID and maybe LAST, then no point in trying to match the individual flags
    if (is_boring_syscall_param_flags(pfixup_flags))
      goto loop_continue;

    STATISTIC_UPDATE() {
      if (in_external_vmcall())
        st_vmcall_arg_needed_fixups++;
    }

    u64 *parg = &args[arg_index];
    g_is_running_nested_fixups = true;

    int result;
    if (FLAG_ISSET(pfixup_flags, SYSCALL_PARAM_IOVEC)) {
      // the length of the iovec array seems to be always passed in as the next argument
      size_t num_iovecs = (size_t)args[arg_index + 1];
      result = fixup_iovec_array(REF (struct iovec **)parg, num_iovecs);
    } else if (FLAG_ISSET(pfixup_flags, SYSCALL_PARAM_PTR_PTR)) {
      result = fixup_ptr_ptr_nullterm(REF (void ***)parg);
    } else if (FLAG_ISSET(pfixup_flags, SYSCALL_PARAM_MSGHDR)) {
      result = fixup_msghdr(REF (struct msghdr **)parg);
    } else if (FLAG_ISSET(pfixup_flags, SYSCALL_PARAM_USER_PTR)) {
      // NOTE: all of the above fixups already take care of this, so we only want to do raw fixup_ptr() if that's the ONLY thing we have to do
      g_is_running_nested_fixups = false;
      result = fixup_ptr(REF (void **)parg, OUT_IGNORE);
    }

    g_is_running_nested_fixups = false;

    if (result < 0) {
      final_result--;

      #if DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO
        const struct syscall_info *sinfo = syscall_get_info(syscallno);

        LOG("Failed to fixup syscall %s arg %lu (%s %s): ",
          sinfo->name,
          arg_index,
          sinfo->params[arg_index].name,
          sinfo->params[arg_index].type);
      #else
        LOG("Failed to fixup syscall %lu arg %lu: ", syscallno, arg_index);
      #endif

      syscall_log(syscallno, args);

      // note: we specifically don't want to stop if we encounter a failure, we'll just try to fixup everything we can
    }

loop_continue:
    /*if (!(arg_index < SYSCALL_MAX_ARGS - 1 || FLAG_ISSET(pfixup_flags, SYSCALL_PARAM_LAST)))
      syscall_log(syscallno, args);*/

    ASSERT(arg_index < SYSCALL_MAX_ARGS - 1 || FLAG_ISSET(pfixup_flags, SYSCALL_PARAM_LAST), "SYSCALL_PARAM_LAST didn't appear on the last parameter's flags!!");

    if (FLAG_ISSET(pfixup_flags, SYSCALL_PARAM_LAST))
      break;
  } // end for

  return final_result;
}
