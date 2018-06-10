#ifndef DANGLESS_COMMON_H
#define DANGLESS_COMMON_H

#include "dangless/common/types.h"
#include "dangless/common/dprintf.h"
#include "dangless/common/assert.h"
#include "dangless/common/util.h"

// These are probably not useful enough to keep around.
#define KB (1024uL)
#define MB (1024uL * KB)
#define GB (1024uL * MB)

// For some reason, when compiling in release mode, off_t is undefined in libdune/dune.h.
#if DANGLESS_CONFIG_PROFILE == release
typedef ptrdiff_t off_t;
#endif

#endif
