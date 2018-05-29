#ifndef DANGLESS_SYSCALLMETA_H
#define DANGLESS_SYSCALLMETA_H

#include <stddef.h>

#define SYSCALL_MAX_NUM_ARGS 6

extern int g_syscall_ptr_args[][SYSCALL_MAX_NUM_ARGS + 1];
extern size_t g_syscall_ptr_args_len;

#endif
