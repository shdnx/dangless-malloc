#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> // for dprintf(), etc.
#include <signal.h> // for ASSERT()

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
#define ROUND_UP(A, N) ((A) + (N) - (A) % (N))

#define dprintf(...) fprintf(stderr, __VA_ARGS__)
#define vdprintf(...) \
  do { \
    dprintf("[%s:%d] %s: ", __FILE__, __LINE__, __func__); \
    dprintf(__VA_ARGS__); \
  } while (0)

#define ASSERT(COND, ...) \
  do { \
    if (!(COND)) { \
      vdprintf("Assertion failed: %s\n", #COND); \
      dprintf(__VA_ARGS__); \
      raise(SIGINT); \
    } \
  } while (0)

#define ASSERT0(COND) \
  ASSERT(COND, "Assertion failure in %s at %s:%d", __func__, __FILE__, __LINE__)

#define UNREACHABLE(...) \
  do { \
    dprintf("Unreachable reached in %s at %s:%d: ", __func__, __FILE__, __LINE__); \
    dprintf(__VA_ARGS__); \
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

#endif // COMMON_H
