#ifndef DANGLESS_SYSCALLMETA_H
#define DANGLESS_SYSCALLMETA_H

#include "dangless/common.h"
#include "dangless/config.h"

enum {
  SYSCALL_MAX_ARGS = 6
};

#if DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO

  struct syscall_param_info {
    index_t pos;
    const char *type;
    const char *name;
    bool is_user_ptr;
  };

  struct syscall_info {
    u64 number;
    const char *name;

    size_t num_params;
    struct syscall_param_info params[SYSCALL_MAX_ARGS];

    const char *signature;
  };

  const struct syscall_info *syscall_get_info(index_t syscallno);

  static inline const char *syscall_get_name(index_t syscallno) {
    const struct syscall_info *info = syscall_get_info(syscallno);
    return info ? info->name : NULL;
  }

#endif // DANGLESS_CONFIG_SYSCALLMETA_HAS_INFO

void syscall_log(u64 syscallno, u64 args[]);

#endif
