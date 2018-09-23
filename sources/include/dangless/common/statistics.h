#ifndef DANGLESS_COMMON_STATISTICS_H
#define DANGLESS_COMMON_STATISTICS_H

#include "dangless/config.h"
#include "dangless/common/util.h"

int dangless_report_resource_usage(void);
void dangless_report_statistics(void);

enum statistic_type {
  ST_COUNTER
};

struct statistic {
  const char *name;
  int line;

  enum statistic_type type;
  void *data_ptr;
  size_t data_size;

  struct statistic *next;
};

struct statistic_file {
  bool is_linked;
  const char *filepath;

  struct statistic *first;
  struct statistic *last;
  size_t num_statistics;

  struct statistic_file *next;
};

bool statistic_file_register(struct statistic_file *stfile);
void statistic_register(struct statistic_file *stfile, struct statistic *stat);

static struct statistic_file statistic_this_file;

#ifdef __STDC_NO_ATOMICS__
  #warning "This compiler doesn't support C11 atomic types - statistics may be unreliable"

  #define DANGLESS_ATOMIC(TY) TY
#else
  #define DANGLESS_ATOMIC(TY) _Atomic(TY)
#endif

#if DANGLESS_CONFIG_COLLECT_STATISTICS
  #define STATISTIC_DEFINE_COUNTER(NAME) \
    static DANGLESS_ATOMIC(size_t) NAME; \
    \
    static struct statistic CONCAT2(_stat_, NAME) = { \
      .name = #NAME, \
      .line = __LINE__, \
      .type = ST_COUNTER, \
      .data_ptr = &NAME, \
      .data_size = sizeof(NAME) \
    }; \
    \
    __attribute__((constructor)) \
    static void CONCAT2(_stat_register_, NAME)(void) { \
      if (statistic_file_register(&statistic_this_file)) { \
        statistic_this_file.filepath = __FILE__; \
      } \
      \
      statistic_register(&statistic_this_file, &CONCAT2(_stat_, NAME)); \
    }

  #define STATISTIC_UPDATE() /* empty */
#else
  // we still need to define the variables, since the code has to compile - hopefully it'll be eliminated by the compiler/linker
  #define STATISTIC_DEFINE_COUNTER(NAME) static size_t NAME;
  #define STATISTIC_UPDATE() if (false)
#endif

#endif
