#include <sys/time.h>
#include <sys/resource.h>

#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/statistics.h"
#include "dangless/common/printf_nomalloc.h"
#include "dangless/common/assert.h"

// TODO: use queue.h instead
struct statistic_file *g_statistics_file_first;
struct statistic_file *g_statistics_file_last;

#define PRINT(...) fprintf_nomalloc(stderr, __VA_ARGS__)

bool statistic_file_register(struct statistic_file *stfile) {
  if (stfile->is_linked)
    return false;

  stfile->is_linked = true;

  if (!g_statistics_file_first) {
    g_statistics_file_first = stfile;
  } else {
    g_statistics_file_last->next = stfile;
  }

  g_statistics_file_last = stfile;
  return true;
}

void statistic_register(struct statistic_file *stfile, struct statistic *stat) {
  if (!stfile->first) {
    stfile->first = stat;
  } else {
    stfile->last->next = stat;
  }

  stfile->last = stat;
  stfile->num_statistics++;
}

void statistic_print(const struct statistic *st) {
  switch (st->type) {
  case ST_COUNTER:
    PRINT("%s = %zu\n", st->name, *(unsigned long *)st->data_ptr);
    break;

  default: UNREACHABLE("Unknown statistic type!");
  }
}

int dangless_report_resource_usage(void) {
  struct rusage usage;
  int result = getrusage(RUSAGE_SELF, OUT &usage);
  if (result < 0) {
    perror("getusage() failed");
    return result;
  }

  PRINT("\n[Resource usage]\n");
  PRINT("user CPU time: %lds %ldu\n", usage.ru_utime.tv_sec, usage.ru_utime.tv_usec);
  PRINT("system CPU time: %lds %ldu\n", usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
  PRINT("max RSS: %ld KB\n", usage.ru_maxrss);
  return 0;
}

void dangless_report_statistics(void) {
  PRINT("\n[Statistics]\n");

  if (!g_statistics_file_first) {
    PRINT("No statistics have been registered!\n");
    return;
  }

  for (const struct statistic_file *stfile = g_statistics_file_first;
       stfile;
       stfile = stfile->next) {
    PRINT("\n[%s]:\n", stfile->filepath);

    for (const struct statistic *st = stfile->first;
         st;
         st = st->next) {
      statistic_print(st);
    }
  }
}

#if DANGLESS_CONFIG_COLLECT_STATISTICS
  __attribute__((destructor))
  void dangless_autoreport(void) {
    dangless_report_resource_usage();
    dangless_report_statistics();
  }
#endif
