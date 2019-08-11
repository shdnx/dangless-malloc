#ifndef DANGLESS_DUNE_VMCALL_FIXUP_INFO_H
#define DANGLESS_DUNE_VMCALL_FIXUP_INFO_H

#include "dangless/common/types.h"
#include "dangless/common/util.h"
#include "dangless/syscallmeta.h"

enum vmcall_param_fixup_type {
  // terminates the parameter list; must be 0
  VMCALL_PARAM_END = 0,

  // indicates that the parameter does not require fixing up (i.e. is not a pointer)
  VMCALL_PARAM_NONE,

  VMCALL_PARAM_FLAT_PTR,
  VMCALL_PARAM_IOVEC,
  VMCALL_PARAM_PTR_PTR,
  VMCALL_PARAM_MSGHDR
} __attribute__((packed));

const char *vmcall_param_fixup_type_str(enum vmcall_param_fixup_type type);

struct vmcall_param_fixup_info {
  enum vmcall_param_fixup_type fixup_type;
};

struct vmcall_fixup_info {
  struct vmcall_param_fixup_info params[SYSCALL_MAX_ARGS];
};

const struct vmcall_fixup_info *vmcall_get_fixup_info(u64 syscallno);

#endif
