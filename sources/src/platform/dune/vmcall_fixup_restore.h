#ifndef DANGLESS_DUNE_VMCALL_FIXUP_RESTORE_H
#define DANGLESS_DUNE_VMCALL_FIXUP_RESTORE_H

#include "dangless/common/types.h"

void vmcall_fixup_restore_init(void);

void vmcall_fixup_restore_on_return(void **location, void *original_value);
void vmcall_fixup_restore_originals(void);

#endif
