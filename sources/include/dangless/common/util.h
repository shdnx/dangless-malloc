#ifndef DANGLESS_COMMON_UTIL_H
#define DANGLESS_COMMON_UTIL_H

#include <stdbool.h>

#define LIKELY(COND) __builtin_expect((COND), 1)
#define UNLIKELY(COND) __builtin_expect((COND), 0)

#define THREAD_LOCAL __thread

#define EXPAND(V) V

#define _DO_STRINGIFY(V) #V
#define STRINGIFY(V) _DO_STRINGIFY(V)

#define __DO_CONCAT2(A, B) A##B
#define CONCAT2(A, B) __DO_CONCAT2(A, B)

#define __DO_CONCAT3(A, B, C) A##B##C
#define CONCAT3(A, B, C) __DO_CONCAT3(A, B, C)

#define FLAG_ISSET(BITSET, BIT) ((bool)((BITSET) & (BIT)))

#define MIN(A, B) ((A) < (B) ? (A) : (B))
#define MAX(A, B) ((A) > (B) ? (A) : (B))

// Round down A to the nearest multiple of N. If A is already a multiple of N, the result is A.
#define ROUND_DOWN(A, N) ((A) - (A) % (N))

// Round up A to the nearest multiple of N. If A is already a multiple of N, the result is A.
#define ROUND_UP(A, N) (((A) + (N) - 1) / (N) * (N))

#define ARRAY_LENGTH(ARR) (sizeof(ARR) / sizeof(ARR[0]))

#endif
