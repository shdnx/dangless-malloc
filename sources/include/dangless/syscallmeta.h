#ifndef DANGLESS_SYSCALLMETA_H
#define DANGLESS_SYSCALLMETA_H

#include "dangless/common.h"

#define SYSCALL_MAX_NUM_ARGS 6

struct syscall_info {
  const char *name;
  const char *return_type;

  size_t num_params;
  const char *param_types[SYSCALL_MAX_NUM_ARGS];

  const char *signature;
};

const struct syscall_info *syscall_get_info(index_t syscallno);

static inline const char *syscall_get_name(index_t syscallno) {
  const struct syscall_info *info = syscall_get_info(syscallno);
  return info ? info->name : NULL;
}

const int *syscall_get_userptr_params(index_t syscallno);

#endif
