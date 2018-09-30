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
#include "dangless/common/debug.h"
#include "dangless/queue.h"

#define USE_GROUPS 0

#if DANGLESS_CONFIG_DEBUG_PERF_EVENTS
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

typedef void (* perf_event_config_func)(struct perf_event_attr *this);

struct perf_event_desc {
  const char *name;
  int fd;
  u64 id;
  bool initialized;
  perf_event_config_func config_func;
  struct perf_event_attr attrs;

  LIST_ENTRY(perf_event_desc) list;
};

#if USE_GROUPS
  struct perf_event_data {
    u64 value;      /* The value of the event */
    u64 id;         /* if PERF_FORMAT_ID */
  };

  struct perf_event_group_data {
    u64 nr;            /* The number of events */
    u64 time_enabled;  /* if PERF_FORMAT_TOTAL_TIME_ENABLED */
    u64 time_running;  /* if PERF_FORMAT_TOTAL_TIME_RUNNING */
    struct perf_event_data values[];
  };
#else
  struct perf_event_data {
    u64 value;         /* The value of the event */
    u64 time_enabled;  /* if PERF_FORMAT_TOTAL_TIME_ENABLED */
    u64 time_running;  /* if PERF_FORMAT_TOTAL_TIME_RUNNING */
    u64 id;            /* if PERF_FORMAT_ID */
  };
#endif

static LIST_HEAD(, perf_event_desc) g_pe_list = LIST_HEAD_INITIALIZER(&g_pe_list);
static size_t g_pe_list_length; // all events
static size_t g_pe_count; // enabled events
static int g_group_leader_fd = -1;

static void perf_event_register(struct perf_event_desc *pe) {
  LOG("Registering performance event %s...\n", pe->name);
  pe->config_func(&pe->attrs);

  LIST_INSERT_HEAD(&g_pe_list, pe, list);
  g_pe_list_length++;
}

#if USE_GROUPS
  #define PE_READ_FORMAT PERF_FORMAT_GROUP | PERF_FORMAT_ID | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING
#else
  #define PE_READ_FORMAT PERF_FORMAT_ID | PERF_FORMAT_TOTAL_TIME_ENABLED | PERF_FORMAT_TOTAL_TIME_RUNNING
#endif

#if DANGLESS_CONFIG_ENABLE_PERF_EVENTS
  #define DEFINE_PERF_COUNTER(NAME) \
    static void CONCAT2(_pe_configure_, NAME)(struct perf_event_attr *this); \
    \
    static struct perf_event_desc CONCAT2(g_pe_, NAME) = { \
      .name = #NAME, \
      .config_func = &CONCAT2(_pe_configure_, NAME), \
      .attrs = { \
        .size = sizeof(struct perf_event_attr), \
        .disabled = 1, \
        .exclude_kernel = 0, \
        .exclude_user = 0, \
        .exclude_hv = 0, \
        .read_format = PE_READ_FORMAT \
      } \
    }; \
    \
    __attribute__((constructor)) \
    void CONCAT2(_register_pe_, NAME)(void) { \
      perf_event_register(&CONCAT2(g_pe_, NAME)); \
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

DEFINE_PERF_COUNTER(tlb_data_prefetch_access) {
  this->type = PERF_TYPE_HW_CACHE;
  this->config = PERF_COUNT_HW_CACHE_DTLB
                  | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8)
                  | (PERF_COUNT_HW_CACHE_RESULT_ACCESS << 16);
}

DEFINE_PERF_COUNTER(tlb_data_prefetch_miss) {
  this->type = PERF_TYPE_HW_CACHE;
  this->config = PERF_COUNT_HW_CACHE_DTLB
                  | (PERF_COUNT_HW_CACHE_OP_PREFETCH << 8)
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
  LIST_FOREACH(pe, &g_pe_list, list) {
    LOG("Initializing perf event %s...\n", pe->name);

    pe->fd = perf_event_open(
      &pe->attrs,
      /*pid=*/0,
      /*cpu=*/-1,
      #if USE_GROUPS
        /*group_fd=*/g_group_leader_fd,
      #else
        /*group_fd=*/-1,
      #endif
      /*flags=*/0
    );

    if (pe->fd == -1) {
      #if DANGLESS_CONFIG_DEBUG_PERF_EVENTS
        LOG("Failed to initialize performance event %s: ", pe->name);
        perror("");
      #endif

      continue;
    }

    if (ioctl(pe->fd, PERF_EVENT_IOC_ID, &pe->id) < 0) {
      #if DANGLESS_CONFIG_DEBUG_PERF_EVENTS
        LOG("Failed to get performance event ID for %s (fd = %d): ", pe->name, pe->fd);
        perror("");
      #endif

      continue;
    }

    LOG("Perf event initialized with fd = %d and id = %lu\n", pe->fd, pe->id);

    if (g_group_leader_fd == -1)
      g_group_leader_fd = pe->fd;

    pe->initialized = true;
  }

  // enable all performance events
  LIST_FOREACH(pe, &g_pe_list, list) {
    if (!pe->initialized)
      continue;

    if (ioctl(pe->fd, PERF_EVENT_IOC_RESET, 0) < 0) {
      #if DANGLESS_CONFIG_DEBUG_PERF_EVENTS
        LOG("Failed to reset perf event %s", pe->name);
        perror("");
      #endif

      pe->initialized = false;
      continue;
    }

    if (ioctl(pe->fd, PERF_EVENT_IOC_ENABLE, 0) < 0) {
      #if DANGLESS_CONFIG_DEBUG_PERF_EVENTS
        LOG("Failed to enable perf event %s", pe->name);
        perror("");
      #endif

      pe->initialized = false;
      continue;
    }

    LOG("Perf event enabled: %s\n", pe->name);
    g_pe_count++;
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

static void perfevent_print(struct perf_event_desc *pe, u64 count, u64 time_enabled, u64 time_running) {
  PRINT("%s = %lu\n", pe->name, (unsigned long)(count * ((double)time_enabled / time_running)));
  PRINT("\t(raw count: %lu, running %u%%: %lu / %lu enabled)\n", count, (unsigned)((double)time_running / time_enabled * 100), time_running, time_enabled);
}

void perfevents_finalize(void) {
  if (g_group_leader_fd == -1) {
    PRINT("No performance events to report!\n");
    return;
  }

  // disable all performance events
  struct perf_event_desc *pe;
  LIST_FOREACH(pe, &g_pe_list, list) {
    if (!pe->initialized)
      continue;

    ioctl(pe->fd, PERF_EVENT_IOC_DISABLE, 0);
  }

  // report all performance events
  PRINT("\n[Performance events]\n");

  #if USE_GROUPS
    char data_buffer[sizeof(struct perf_event_group_data) + g_pe_count * sizeof(struct perf_event_data)];
    ssize_t rresult = read(g_group_leader_fd, data_buffer, sizeof(data_buffer));

    struct perf_event_group_data *group_data = (struct perf_event_group_data *)data_buffer;

    if (rresult == 0) {
      PRINT("Error: read EOF\n");
    } else if (rresult < 0) {
      perror("Read error: ");
    } else {
      ASSERT0(group_data->nr == g_pe_count);
      ASSERT0(rresult == sizeof(data_buffer));

      //PRINT("Time enabled: %lu, time running: %lu\n", group_data->time_enabled, group_data->time_running);

      for (size_t i = 0; i < g_pe_count; i++) {
        LIST_FOREACH(pe, &g_pe_list, list) {
          if (pe->id != group_data->values[i].id)
            continue;

          perfevent_print(pe, group_data->values[i].value, group_data->time_enabled, group_data->time_running);
        }
      }
    }
  #else
    LIST_FOREACH(pe, &g_pe_list, list) {
      if (!pe->initialized)
        continue;

      struct perf_event_data event_data;
      ssize_t rresult = read(pe->fd, &event_data, sizeof(event_data));

      if (rresult == 0) {
        PRINT("%s: EOF read\n", pe->name);
      } else if (rresult < 0) {
        PRINT("%s: ", pe->name);
        perror("Read error: ");
      } else {
        perfevent_print(pe, event_data.value, event_data.time_enabled, event_data.time_running);
      }
    }
  #endif

  // print events that weren't enabled
  LIST_FOREACH(pe, &g_pe_list, list) {
    if (pe->initialized)
      continue;

    PRINT("%s: not run\n", pe->name);
  }

  // close all perf events
  LIST_FOREACH(pe, &g_pe_list, list) {
    if (!pe->initialized)
      continue;

    close(pe->fd);
    pe->initialized = false;
  }

  g_pe_count = 0;
}
