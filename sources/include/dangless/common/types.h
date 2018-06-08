#ifndef DANGLESS_COMMON_TYPES_H
#define DANGLESS_COMMON_TYPES_H

#include <stdbool.h> // re-exported
#include <stdint.h> // int8_t, int16_t, etc.
#include <stddef.h> // ptrdiff_t

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

// Indicates that a pointer function parameter or return type points to an array, not just a single object.
#define ARRAY /* empty */

#endif
