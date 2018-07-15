#ifndef DANGLESS_DUNE_VMCALL_FIXUP_H
#define DANGLESS_DUNE_VMCALL_FIXUP_H

#include "dangless/common/types.h"

int vmcall_fixup_args(u64 syscallno, u64 args[]);

#endif
