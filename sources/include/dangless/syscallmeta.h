#ifndef DANGLESS_SYSCALLMETA_H
#define DANGLESS_SYSCALLMETA_H

#include "dangless/common.h"

#define SYSCALL_MAX_NUM_ARGS 6

const int *syscall_get_userptr_params(index_t syscallno);
const char *syscall_get_name(index_t syscallno);

#endif
