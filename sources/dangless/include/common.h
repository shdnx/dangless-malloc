#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> // for stderr, fprintf()
#include <signal.h> // for use by ASSERT()

// libc's printf() et al MAY allocate memory using malloc() et al. In practice, printing only to stderr using glibc this doesn't seem to ever happen, but I want to be sure.
// Even if such a call would result in an allocation, we're fine so long as the call occurs during a hook execution AFTER a HOOK_ENTER() call is successful. For all printing and logging purposes, code that may run before or during HOOK_ENTER() MUST use the functions from printf_nomalloc.h.
#include "printf_nomalloc.h"

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

// Used to indicate function parameters
#define OUT /* empty */
#define INOUT /* empty */

#define KB (1024uL)
#define MB (1024uL * KB)
#define GB (1024uL * MB)

#define EVAL(V) V
#define STRINGIFY(V) #V

#define __DO_CONCAT2(A, B) A##B
#define CONCAT2(A, B) __DO_CONCAT2(A, B)

#define __DO_CONCAT3(A, B, C) A##B##C
#define CONCAT3(A, B, C) __DO_CONCAT3(A, B, C)

#define FLAG_ISSET(BITSET, BIT) ((bool)((BITSET) & (BIT)))

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

// Round down A to the nearest multiple of N.
#define ROUND_DOWN(A, N) ((A) - (A) % (N))
#define ROUND_UP(A, N) (((A) + (N) - 1) / (N) * (N))

#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#define dprintf_nomalloc(...) fprintf_nomalloc(stderr, __VA_ARGS__)

void _print_caller_info(const char *file, const char *func, int line);
void _print_caller_info_nomalloc(const char *file, const char *func, int line);

#define _CALLER_INFO_ARGS \
  __FILE__, __func__, __LINE__

#define vdprintf(...) \
  do { \
    _print_caller_info(_CALLER_INFO_ARGS); \
    dprintf(__VA_ARGS__); \
  } while (0)

#define vdprintf_nomalloc(...) \
  do { \
    _print_caller_info_nomalloc(_CALLER_INFO_ARGS); \
    dprintf_nomalloc(__VA_ARGS__); \
  } while (0)

#define ASSERT(COND, ...) \
  do { \
    if (!(COND)) { \
      vdprintf_nomalloc("Assertion failed: %s\n", #COND); \
      dprintf_nomalloc(__VA_ARGS__); \
      raise(SIGINT); \
    } \
  } while (0)

#define ASSERT0(COND) \
  ASSERT(COND, "Assertion failure in %s at %s:%d", __func__, __FILE__, __LINE__)

#define UNREACHABLE(...) \
  do { \
    dprintf_nomalloc("Unreachable reached in %s at %s:%d: ", __func__, __FILE__, __LINE__); \
    dprintf_nomalloc(__VA_ARGS__); \
    __builtin_unreachable(); \
  } while (0)

// TODO: test for GCC 4.6 or a new enough Clang to have _Static_assert
#define STATIC_ASSERT(COND, MSG) _Static_assert(COND, MSG)

#define STATIC_ASSERT0(COND) STATIC_ASSERT(COND, "Static assertion failure at " __FILE__ ":" STRINGIFY(__LINE__))

#define FUNC_STATIC_ASSERT(COND, MSG) STATIC_ASSERT(COND, MSG)

// Based on: https://stackoverflow.com/a/3385694/128240
/*#define STATIC_ASSERT(COND, MSG) \
  typedef char static_assertion_##MSG[(!!(COND)) * 2 - 1]

#define STATIC_ASSERT0(COND) \
  STATIC_ASSERT(COND, CONCAT2(static_assert_at_line_, __LINE__))

#define FUNC_STATIC_ASSERT(COND, MSG) { \
    extern void static_assert_fail() __attribute__((error(MSG))); \
    if (!(COND)) \
      static_assert_fail(); \
  }*/

#define LIKELY(COND) __builtin_expect((COND), 1)
#define UNLIKELY(COND) __builtin_expect((COND), 0)

#define THREAD_LOCAL __thread

#endif // COMMON_H
