#ifndef DANGLESS_COMMON_H
#define DANGLESS_COMMON_H

#include <stdint.h> // int8_t, int16_t, etc.
#include <stddef.h> // ptrdiff_t
#include <stdbool.h>
#include <stdio.h> // for stderr, fprintf()

// libc's printf() et al MAY allocate memory using malloc() et al. In practice, printing only to stderr using glibc this doesn't seem to ever happen, but I want to be sure.
// Even if such a call would result in an allocation, we're fine so long as the call occurs during a hook execution AFTER a HOOK_ENTER() call is successful. For all printing and logging purposes, code that may run before or during HOOK_ENTER() MUST use the functions from printf_nomalloc.h.
#include "dangless/printf_nomalloc.h"

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef ptrdiff_t index_t;

// Indicate function parameters that are used only to pass information from the callee to the caller. Only used to signal intent, has no effect on the code.
#define OUT /* empty */

// At a function call site, use instead of a NULL argument to indicate that the OUT parameter is going to be ignored for clearer intent.
#define OUT_IGNORE NULL

// Indicates function parameters that are meant to pass data both ways: from the caller to the callee and vica versa. Only used to signal intent, has no effect on the code.
#define REF /* empty */

// Indicates that a pointer or pointer-like function parameter is expected not to be NULL. Only used to signal intent, has no effect on the code.
#define NOT_NULL /* empty */

#define LIKELY(COND) __builtin_expect((COND), 1)
#define UNLIKELY(COND) __builtin_expect((COND), 0)

#define THREAD_LOCAL __thread

// These are probably not useful enough to keep around.
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

void _assert_fail(void);

#ifdef NDEBUG
  #define ASSERT(COND, ...) /* empty */
  #define ASSERT0(COND) /* empty */
#else
  #define _ASSERT_IMPL(COND, MSG_PRE, ...) \
    do { \
      if (UNLIKELY(!(COND))) { \
        vdprintf_nomalloc("Assertion %s failed: ", #COND); \
        dprintf_nomalloc(__VA_ARGS__); \
        _assert_fail(); \
      } \
    } while (0)

  #define ASSERT(COND, ...) _ASSERT_IMPL(COND, ": ", __VA_ARGS__)

  // TODO: this is an awful name, but I can't think of a better one for the life of me.
  #define ASSERT0(COND) _ASSERT_IMPL(COND, "!", "")
#endif

#ifdef NDEBUG
  #define UNREACHABLE(...) __builtin_unreachable()
#else
  #define UNREACHABLE(...) \
    do { \
      vdprintf_nomalloc("Unreachable executed: "); \
      dprintf_nomalloc(__VA_ARGS__); \
      __builtin_unreachable(); \
    } while (0)
#endif

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

#endif
