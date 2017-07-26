#ifndef RUMPRUN_H
#define RUMPRUN_H

#include "common.h"

// These macros allow us to use functions defined by rumprun by knowing their offset inside rumprun.o. They use function pointer variables.
// Linking to the rumprun modules is very tricky, we'd basically have to pull in the entire rumprun as well as use its linker script, which sounds super unhealthy and would likely lead to all sort of problems besides the waste.
// Furthermore, rumprun does not support shared libraries, or dlopen() et al, so we have no convenient shortcuts.

// Use RUMPRUN_DECL_FUNC() to declare rumprun functions and RUMPRUN_DEF_FUNC() to define them using their offset inside rumprun.o.
// RUMPRUN_DECL_FUNC() is optional, and only needed if you want to expose a rumprun function to another translation unit via a header file. It is, for all intents and purposes, a variable declaration.
// RUMPRUN_DEF_FUNC() is mandatory. It is, in effect, a variable definition.
// The rumprun functions defined by RUMPRUN_DEF_FUNC() can be then called via the RUMPKERN_FUNC() macro.
//
// For example:
//
//  void *page_alloc(void) {
//    static RUMPRUN_DEF_FUNC(0xce60, void *, bmk_pgalloc, int /*order*/);
//    return RUMPRUN_FUNC(bmk_pgalloc)(/*order=*/0);
//  }

// TODO: values such as RUMPRUN_TEXT_BASE would not be baked in, but supplied by the build system
// TODO: also ideally, the user would not have to specify the symbol offsets manually, but an automated post-processing script would determine them
#define RUMPRUN_TEXT_BASE 0x102000

#define __RUMPRUN_SYM_NAME(NAME) __rumprunsym_##NAME
#define __RUMPRUN_FUNC_PTR(RETTYPE, NAME, ...) RETTYPE(*__RUMPRUN_SYM_NAME(NAME))(__VA_ARGS__)

#define RUMPRUN_DECL_FUNC(RETTYPE, NAME, ...) \
  extern __RUMPRUN_FUNC_PTR(RETTYPE, NAME, __VA_ARGS__)

#define RUMPRUN_DEF_FUNC(OFFSET, RETTYPE, NAME, ...) \
  __RUMPRUN_FUNC_PTR(RETTYPE, NAME, __VA_ARGS__) = \
    ((RETTYPE(*)(__VA_ARGS__))(RUMPRUN_TEXT_BASE + OFFSET))

#define RUMPRUN_FUNC(NAME) __RUMPRUN_SYM_NAME(NAME)

// TODO: offset? probably different offset for bss, rodata, data
/*#define RUMPRUN_DECL_VAR(OFFSET, TYPE, NAME) \
    static TYPE *__RUMPRUN_SYM_NAME(NAME) = ((TYPE *)(OFFSET));

#define RUMPRUN_VAR(NAME) (*__RUMPRUN_SYM_NAME(NAME))*/

#endif // RUMPRUN_H