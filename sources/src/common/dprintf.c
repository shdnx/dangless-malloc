#define _GNU_SOURCE

#include <stdarg.h>
#include <stdio.h>
#include <string.h> // strlen

#include <unistd.h>
#include <sys/types.h> // gettid
#include <sys/syscall.h> // syscall

#include "dangless/common/types.h"
#include "dangless/common/util.h"

// libc's printf() et al MAY allocate memory using malloc() et al. In practice, printing only to stderr using glibc this doesn't seem to ever happen, but I want to be sure.
// Even if such a call would result in an allocation, we're fine so long as the call occurs during a hook execution AFTER a HOOK_ENTER() call is successful. For all printing and logging purposes, code that may run before or during HOOK_ENTER() MUST use the functions from printf_nomalloc.h.
#include "dangless/common/printf_nomalloc.h"

enum {
  INDENT_SIZE = 4
};

bool dprintf_enabled = true;
int dprintf_scope_depth = 0;

static bool g_had_newline = false;

#define _DPRINTF_IMPL(PRINTF, FMT, ARGS) \
  do { \
    if (g_had_newline) \
      PRINTF(stderr, "%*s", dprintf_scope_depth * INDENT_SIZE, ""); \
    \
    EXPAND(CONCAT2(v, PRINTF))(stderr, FMT, ARGS); \
    \
    g_had_newline = ((FMT)[strlen((FMT)) - 1] == '\n'); \
  } while (0)

void _dprintf(const char *restrict format, ...) {
  if (!dprintf_enabled)
    return;

  va_list args;
  va_start(args, format);
  _DPRINTF_IMPL(fprintf, format, args);
  va_end(args);
}

void _dprintf_nomalloc(const char *restrict format, ...) {
  if (!dprintf_enabled)
    return;

  va_list args;
  va_start(args, format);
  _DPRINTF_IMPL(fprintf_nomalloc, format, args);
  va_end(args);
}

#define _PRINT_CALLER_INFO_IMPL(PRINTF) \
  do { \
    const pid_t thread_id = syscall(SYS_gettid); \
    PRINTF("[%s:%d]{T %d} %s: ", file, line, thread_id, func); \
  } while (0)

void _print_caller_info(const char *file, const char *func, int line) {
  _PRINT_CALLER_INFO_IMPL(_dprintf);
}

void _print_caller_info_nomalloc(const char *file, const char *func, int line) {
  _PRINT_CALLER_INFO_IMPL(_dprintf_nomalloc);
}

#undef _PRINT_CALLER_INFO_IMPL
