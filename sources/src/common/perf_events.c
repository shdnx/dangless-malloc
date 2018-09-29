#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/perf_event.h>
#include <asm/unistd.h>

#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/assert.h"
#include "dangless/common/dprintf.h"
#include "dangless/common/printf_nomalloc.h"
#include "dangless/common/util.h"
#include "dangless/queue.h"

#if DANGLESS_CONFIG_DEBUG_PERF_EVENTS
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

struct perf_event_desc {
  const char *name;
  int fd;
  bool initialized;
  struct perf_event_attr attrs;

  LIST_ENTRY(perf_event_desc) list;
};

static LIST_HEAD(, perf_event_desc) g_pe_list = LIST_HEAD_INITIALIZER(&g_pe_list);

#if DANGLESS_CONFIG_ENABLE_PERF_EVENTS
  #define DEFINE_PERF_COUNTER(NAME) \
    static struct perf_event_desc CONCAT2(g_pe_, NAME) = { \
      .name = #NAME, \
      .attrs = { \
        .size = sizeof(struct perf_event_attr), \
        .disabled = 1, \
        .exclude_kernel = 0, \
        .exclude_user = 1, \
        .exclude_hv = 1 \
      } \
    }; \
    \
    static void CONCAT2(_pe_configure_, NAME)(struct perf_event_attr *this); \
    \
    __attribute__((constructor)) \
    void CONCAT2(_register_pe_, NAME)(void) { \
      LOG("Registering performance event %s...\n", #NAME); \
      CONCAT2(_pe_configure_, NAME)(&CONCAT2(g_pe_, NAME).attrs); \
      LIST_INSERT_HEAD(&g_pe_list, &CONCAT2(g_pe_, NAME), list); \
    } \
    static void CONCAT2(_pe_configure_, NAME)(struct perf_event_attr *this)
#else
  #define DEFINE_PERF_COUNTER(NAME) \
    static void CONCAT2(_pe_dummy_, NAME)(struct perf_event_attr *this)
#endif

DEFINE_PERF_COUNTER(tlb_data_read_access) {
  this->type = PERF_TYPE_HW_CACHE;
  this->config = PERF_COUNT_HW_CACHE_DTLB
                  | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                  | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
}

DEFINE_PERF_COUNTER(tlb_data_read_miss) {
  this->type = PERF_TYPE_HW_CACHE;
  this->config = PERF_COUNT_HW_CACHE_DTLB
                  | (PERF_COUNT_HW_CACHE_OP_READ << 8)
                  | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
}

DEFINE_PERF_COUNTER(tlb_data_write_access) {
  this->type = PERF_TYPE_HW_CACHE;
  this->config = PERF_COUNT_HW_CACHE_DTLB
                  | (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
                  | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
}

DEFINE_PERF_COUNTER(tlb_data_write_miss) {
  this->type = PERF_TYPE_HW_CACHE;
  this->config = PERF_COUNT_HW_CACHE_DTLB
                  | (PERF_COUNT_HW_CACHE_OP_WRITE << 8)
                  | (PERF_COUNT_HW_CACHE_RESULT_MISS << 16);
}

static long perf_event_open(
  struct perf_event_attr *hw_event,
  pid_t pid,
  int cpu,
  int group_fd,
  unsigned long flags
) {
  int ret;
  ret = syscall(__NR_perf_event_open, hw_event, pid, cpu, group_fd, flags);
  return ret;
}

#define dangless_error(...) fprintf_nomalloc(stderr, "[Dangless] " __VA_ARGS__)

void perfevents_init(void) {
  // initialize all performance events
  struct perf_event_desc *pe;
  int group_leader_fd = -1;
  LIST_FOREACH(pe, &g_pe_list, list) {
    LOG("Initializing perf event %s...\n", pe->name);

    pe->fd = perf_event_open(
      &pe->attrs,
      /*pid=*/0,
      /*cpu=*/-1,
      /*group_fd=*/group_leader_fd,
      /*flags=*/0
    );

    if (pe->fd == -1) {
      dangless_error("Failed to initialize performance event %s: ", pe->name);
      perror("");
      continue;
    }

    LOG("Perf event initialized with fd = %d\n", pe->fd);

    if (group_leader_fd == -1)
      group_leader_fd = pe->fd;

    pe->initialized = true;
  }

  // enable all performance events
  LIST_FOREACH(pe, &g_pe_list, list) {
    if (!pe->initialized)
      continue;

    if (ioctl(pe->fd, PERF_EVENT_IOC_RESET, 0) < 0) {
      dangless_error("Failed to reset perf event %s", pe->name);
      perror("");
      continue;
    }

    if (ioctl(pe->fd, PERF_EVENT_IOC_ENABLE, 0) < 0) {
      dangless_error("Failed to enable perf event %s", pe->name);
      perror("");
      continue;
    }

    LOG("Perf event enabled: %s\n", pe->name);
  }

  LOG("Done with initializing all perf events...\n");
}

#if DANGLESS_CONFIG_ENABLE_PERF_EVENTS
  __attribute__((constructor))
  void perfevents_autoinit(void) {
    perfevents_init();
  }
#endif

#define PRINT(...) fprintf_nomalloc(stderr, __VA_ARGS__)

static void perfevent_print(ssize_t result, long long count, struct perf_event_desc *pe) {
  PRINT("%s = ", pe->name);
  if (result == 0) {
    PRINT("(EOF)");
  } else if (result < 0) {
    perror("read error");
  } else {
    ASSERT0(result == sizeof(count));
    PRINT("%lld", count);
  }
  PRINT("\n");
}

void perfevents_finalize(void) {
  // disable all performance events
  struct perf_event_desc *pe;
  LIST_FOREACH(pe, &g_pe_list, list) {
    if (!pe->initialized)
      continue;

    ioctl(pe->fd, PERF_EVENT_IOC_DISABLE, 0);
  }

  // report all performance events
  PRINT("\n[Performance events]\n");
  LIST_FOREACH(pe, &g_pe_list, list) {
    if (!pe->initialized)
      continue;

    long long count;
    ssize_t result = read(pe->fd, &count, sizeof(count));
    close(pe->fd);
    pe->initialized = false;

    perfevent_print(result, count, pe);
  }
}
