#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>
#include <stdbool.h>

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

#endif // COMMON_H