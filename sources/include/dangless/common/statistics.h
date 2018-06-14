#ifndef DANGLESS_COMMON_STATISTICS_H
#define DANGLESS_COMMON_STATISTICS_H

#include <stdio.h>

#include "dangless/config.h"
#include "dangless/common/util.h"
#include "dangless/common/printf_nomalloc.h"

#ifdef __STDC_NO_ATOMICS__
  #warning "This compiler doesn't support C11 atomic types - statistics may be unreliable"

  #define DANGLESS_ATOMIC(TY) TY
#else
  #define DANGLESS_ATOMIC(TY) _Atomic(TY)
#endif

#if DANGLESS_CONFIG_COLLECT_STATISTICS
  #define STATISTIC_DEFINE(TYPE, NAME) \
    static DANGLESS_ATOMIC(TYPE) NAME; \
    \
    __attribute__((destructor)) \
    static void CONCAT2(_stat_report_, NAME)(void) { \
      fprintf_nomalloc(stderr, "STAT [" __FILE__ ":" STRINGIFY(__LINE__) "] " #NAME " = %zu\n", NAME); \
    }

  #define STATISTIC_UPDATE() /* empty */
#else
  // we still need to define the variables, since the code has to compile - hopefully it'll be eliminated by the compiler/linker
  #define STATISTIC_DEFINE(TYPE, NAME) static TYPE NAME;
  #define STATISTIC_UPDATE() if (false)
#endif

#endif
