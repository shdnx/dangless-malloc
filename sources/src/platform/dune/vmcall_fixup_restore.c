#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/assert.h"
#include "dangless/common/dprintf.h"
#include "dangless/common/statistics.h"
#include "dangless/platform/sysmalloc.h"

#include "vmcall_fixup_restore.h"

#if DANGLESS_CONFIG_DEBUG_DUNE_VMCALL_FIXUP_RESTORE
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

struct restoration_record {
  void **location;
  void *value;
};

// TODO: make thread-safe!
static struct restoration_record ARRAY *g_records = NULL;
static size_t g_records_capacity = 0;
static size_t g_record_count = 0;

STATISTIC_DEFINE_COUNTER(st_restore_buffer_highest_capacity);
STATISTIC_DEFINE_COUNTER(st_restore_buffer_grow_count);
STATISTIC_DEFINE_COUNTER(st_restore_record_count);
STATISTIC_DEFINE_COUNTER(st_restore_record_failed_count);
STATISTIC_DEFINE_COUNTER(st_restore_applied_count);

static bool grow_records_buffer(size_t new_capacity) {
  struct restoration_record ARRAY *new_buffer = sysrealloc(g_records, new_capacity * sizeof(struct restoration_record));

  if (new_buffer) {
    LOG("grew restoration buffer capacity from %zu to %zu\n", g_records_capacity, new_capacity);

    g_records = new_buffer;
    g_records_capacity = new_capacity;

    STATISTIC_UPDATE() {
      st_restore_buffer_highest_capacity = g_records_capacity;
      st_restore_buffer_grow_count++;
    }

    return true;
  } else {
    LOG("failed to grow restoration buffer capacity from %zu to %zu - out of memory\n", g_records_capacity, new_capacity);
    return false;
  }
}

void vmcall_fixup_restore_init(void) {
  grow_records_buffer(DANGLESS_CONFIG_DUNE_VMCALL_FIXUP_RESTORE_INITIAL_BUFFER_SIZE);
}

void vmcall_fixup_restore_on_return(void **location, void *original_value) {
  if (g_record_count == g_records_capacity) {
    if (!grow_records_buffer(g_records_capacity * 2)) {
      STATISTIC_UPDATE() {
        st_restore_record_failed_count++;
      }

      return;
    }
  }

  struct restoration_record *record = &g_records[g_record_count++];
  record->location = location;
  record->value = original_value;

  STATISTIC_UPDATE() {
    st_restore_record_count++;
  }

  LOG("recorded pointer location %p to be restored to original value %p upon vmcall return\n", record->location, record->value);
}

void vmcall_fixup_restore_originals(void) {
  if (g_record_count == 0)
    return;

  LOG("restoring original pointers...\n");

  for (size_t i = 0; i < g_record_count; ++i) {
    struct restoration_record *record = &g_records[i];
    *(record->location) = record->value;

    STATISTIC_UPDATE() {
      st_restore_applied_count++;
    }
  }

  LOG("successfully restored %zu pointers\n", g_record_count);
  g_record_count = 0;
}
