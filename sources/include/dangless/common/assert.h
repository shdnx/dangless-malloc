#ifndef DANGLESS_COMMON_ASSERT_H
#define DANGLESS_COMMON_ASSERT_H

#include "dangless/common/util.h"
#include "dangless/common/dprintf.h"

void _dangless_assert_fail(void);

#ifdef NDEBUG
  #define ASSERT(COND, ...) /* empty */
  #define ASSERT0(COND) /* empty */
#else
  #define _ASSERT_IMPL(COND, MSG_PRE, ...) \
    do { \
      if (UNLIKELY(!(COND))) { \
        vdprintf_nomalloc("Assertion %s failed: ", #COND); \
        dprintf_nomalloc(__VA_ARGS__); \
        dprintf_nomalloc("\n"); \
        _dangless_assert_fail(); \
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
      dprintf_nomalloc("\n"); \
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
