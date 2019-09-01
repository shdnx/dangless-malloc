#ifndef DANGLESS_DUNE_VMCALL_FIXUP_INFO_H
#define DANGLESS_DUNE_VMCALL_FIXUP_INFO_H

#include "dangless/common/types.h"
#include "dangless/common/util.h"
#include "dangless/syscallmeta.h"

enum vmcall_param_fixup_type {
  VMCALL_PARAM_NONE, // no need to fix up
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
  i8 num_params;
  struct vmcall_param_fixup_info params[SYSCALL_MAX_ARGS];
};

const struct vmcall_fixup_info *vmcall_get_fixup_info(u64 syscallno);

#endif
