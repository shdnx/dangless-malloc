#define _GNU_SOURCE

#include <stdio.h>

#include <unistd.h>
#include <sys/types.h> // gettid()
#include <sys/syscall.h>

#include "common.h"

void _print_caller_info(const char *file, const char *func, int line) {
  pid_t thread_id = syscall(SYS_gettid);
  dprintf("[%s:%d]{T %d} %s: ", file, line, thread_id, func);
}
