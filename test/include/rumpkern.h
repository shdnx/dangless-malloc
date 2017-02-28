#ifndef RUMPKERN_H
#define RUMPKERN_H

#include "common.h"

// Use RUMPKERN_DECL() to declare functions using their offset inside rumprun.o.
// These functions can be called via the RUMPKERN_REF() macros, e.g.:
//
//    RUMPKERN_DECL(0xce60, void *, bmk_pgalloc, int /*order*/);
//    ...
//    RUMPKERN_REF(bmk_pgalloc)(/*order=*/0);

// TODO: ideally, the user would not have to specify the offset manually, but an automated post-processing script would do that

#define RUMPKERN_TEXT_BASE 0x102000

#define __RUMPKERN_SYM_NAME(NAME) __rumpkernsym_##NAME

#define RUMPKERN_DECL_FUNC(OFFSET, RETTYPE, NAME, ...) \
  static RETTYPE(*__RUMPKERN_SYM_NAME(NAME))(__VA_ARGS__) = \
    ((RETTYPE(*)(__VA_ARGS__))(RUMPKERN_TEXT_BASE + OFFSET))

#define RUMPKERN_FUNC(NAME) __RUMPKERN_SYM_NAME(NAME)

// TODO: offset? probably different offset for bss, rodata, data
#define RUMPKERN_DECL_VAR(OFFSET, TYPE, NAME) \
    static TYPE *__RUMPKERN_SYM_NAME(NAME) = ((TYPE *)(OFFSET));

#define RUMPKERN_VAR(NAME) (*__RUMPKERN_SYM_NAME(NAME))

#endif