#ifndef DANGLESS_COMMON_DEBUG_H
#define DANGLESS_COMMON_DEBUG_H

#include "dangless/common/types.h"

void dump_mem_region(const void *ptr, size_t len);

void dump_var_mem(const char *name, const void *ptr, size_t len);

#define DUMP_VAR_MEM(VARNAME) \
  dump_var_mem(#VARNAME, (const void *)&(VARNAME), sizeof(VARNAME))

#endif
