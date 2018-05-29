#define _GNU_SOURCE

#include <signal.h> // raise, SIGINT

#include <unistd.h>
#include <sys/types.h> // gettid
#include <sys/syscall.h> // syscall

#include "dangless/common.h"

#define _PRINT_CALLER_INFO_IMPL(PRINTFUNCNAME) \
  do { \
    pid_t thread_id = syscall(SYS_gettid); \
    PRINTFUNCNAME("[%s:%d]{T %d} %s: ", file, line, thread_id, func); \
  } while (0)

void _print_caller_info(const char *file, const char *func, int line) {
  _PRINT_CALLER_INFO_IMPL(dprintf);
}

void _print_caller_info_nomalloc(const char *file, const char *func, int line) {
  _PRINT_CALLER_INFO_IMPL(dprintf_nomalloc);
}

void _assert_fail(void) {
  raise(SIGINT);
}
