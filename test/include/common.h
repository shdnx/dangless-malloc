#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

#include "sysmalloc.h"

// Used to indicate function parameters
#define OUT /* empty */
#define INOUT /* empty */

#define KB (1024uL)
#define MB (1024uL * KB)
#define GB (1024uL * MB)

#define EVAL(V) V

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
    dprintf("[%s :%d] %s: ", __FILE__, __LINE__, __func__); \
    dprintf(__VA_ARGS__); \
  } while (0)

#define UNREACHABLE(...) \
  do { \
    dprintf("Unreachable reached in %s at %s:%d: ", __func__, __FILE__, __LINE__); \
    dprintf(__VA_ARGS__); \
    __builtin_unreachable(); \
  } while (0)

#endif // COMMON_H