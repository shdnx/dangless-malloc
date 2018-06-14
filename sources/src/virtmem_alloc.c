#include "dangless/config.h"

#if DANGLESS_CONFIG_SUPPORT_MULTITHREADING
  #include <pthread.h>
#else
  #include "dangless/pthread_mock.h"
#endif

#include "dangless/common.h"
#include "dangless/common/statistics.h"

#include "dangless/queue.h"
#include "dangless/virtmem_alloc.h"
#include "dangless/platform/mem.h"
#include "dangless/platform/sysmalloc.h"

#if DANGLESS_CONFIG_DEBUG_VIRTMEMALLOC
  #define LOG(...) vdprintf(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

STATISTIC_DEFINE(size_t, num_allocations);
STATISTIC_DEFINE(size_t, num_allocations_failed);
STATISTIC_DEFINE(size_t, num_allocated_pages);

struct vp_span {
  vaddr_t start;
  vaddr_t end;

  LIST_ENTRY(vp_span) freelist;
};

static inline size_t span_num_pages(struct vp_span *span) {
  return (span->end - span->start) / PGSIZE;
}

static inline bool span_empty(struct vp_span *span) {
  return span->start == span->end;
}

// TODO: buckets, initially just have 3: 1-page, 2-page (because of the offset thing) and larger

struct vp_freelist {
  // Singly-linked list of vp_span objects, ordered by span->end in ascending order.
  LIST_HEAD(, vp_span) items;

  pthread_mutex_t mutex;

  // only used if DANGLESS_CONFIG_COLLECT_STATISTICS == 1
  size_t nallocs;
};

#define VP_FREELIST_INIT(PTR) { \
    LIST_HEAD_INITIALIZER(&(PTR)->items), \
    PTHREAD_MUTEX_INITIALIZER, \
    /*nallocs=*/0 \
  }

static struct vp_freelist g_freelist_main = VP_FREELIST_INIT(&vp_freelist_main);

static void *freelist_alloc(struct vp_freelist *list, size_t npages) {
  pthread_mutex_lock(&list->mutex);

  struct vp_span *span;
  LIST_FOREACH(span, &list->items, freelist) {
    if (span_num_pages(span) >= npages)
      break;
  }

  if (UNLIKELY(span == LIST_END(&list->items))) {
    LOG("could not allocate %zu pages: freelist empty\n", npages);
    pthread_mutex_unlock(&list->mutex);

    STATISTIC_UPDATE() {
      num_allocations_failed++;
    }

    return NULL;
  }

  STATISTIC_UPDATE() {
    num_allocations++;
    num_allocated_pages += npages;
  }

  ASSERT0(span);
  vaddr_t va = span->start;
  span->start += npages * PGSIZE;

  STATISTIC_UPDATE() {
    list->nallocs++;
  }

  LOG("satisfying %zu page alloc from span %p => 0x%lx\n", npages, span, va);

  if (span_empty(span)) {
    LOG("span %p is now empty, deallocating\n", span);
    LIST_REMOVE(span, freelist);
    FREE(span);
  }

  pthread_mutex_unlock(&list->mutex);
  return (void *)va;
}

static struct vp_span *try_merge_spans(struct vp_span *left, struct vp_span *right) {
  ASSERT0(left->end <= right->start);

  if (left->end != right->start)
    return NULL;

  LOG("merging span %p (%p - %p) with span %p (%p - %p)\n", left, (void *)left->start, (void *)left->end, right, (void *)right->start, (void *)right->end);

  // merge 'right' into 'left'
  left->end = right->end;
  LIST_REMOVE(right, freelist);
  FREE(right);

  return left;
}

static int freelist_free(struct vp_freelist *list, void *p, size_t npages) {
  ASSERT0((vaddr_t)p % PGSIZE == 0);
  ASSERT0(npages > 0);

  vaddr_t start = (vaddr_t)p;
  vaddr_t end = PG_OFFSET(start, npages);

  pthread_mutex_lock(&list->mutex);

  // find the two spans between which the new span would go
  struct vp_span *prev_span = NULL, *next_span;
  LIST_FOREACH(next_span, &list->items, freelist) {
    if (next_span->start >= end)
      break;

    prev_span = next_span;
  }

  // try merging with prev_span
  if (prev_span != NULL && prev_span->end == start) {
    LOG("merging freed range %p - %p into prev span %p (%p - %p)\n", (void *)start, (void *)end, prev_span, (void *)prev_span->start, (void *)prev_span->end);

    prev_span->end = end;

    // try merging prev_span with next_span
    if (next_span != LIST_END(&list->items))
      try_merge_spans(prev_span, next_span);

    pthread_mutex_unlock(&list->mutex);
    return 0;
  }

  // try merging with next_span
  if (next_span != LIST_END(&list->items) && next_span->start == end) {
    LOG("merging freed range %p - %p into next span %p (%p - %p)\n", (void *)start, (void *)end, next_span, (void *)next_span->start, (void *)next_span->end);

    next_span->start = start;

    // try merging prev_span with next_span
    if (prev_span)
      try_merge_spans(prev_span, next_span);

    pthread_mutex_unlock(&list->mutex);
    return 0;
  }

  // failed to merge into existing spans, so we'll have to create a new span
  struct vp_span *span = MALLOC(struct vp_span);
  if (UNLIKELY(!span)) {
    LOG("could not allocate vp_span: out of memory?\n");
    pthread_mutex_unlock(&list->mutex);
    return -1;
  }

  span->start = start;
  span->end = end;

  LOG("new span %p: %p - %p (%zu pages)\n", span, (void *)span->start, (void *)span->end, span_num_pages(span));

  // insert the new span
  if (prev_span) {
    LIST_INSERT_AFTER(prev_span, span, freelist);
  } else if (next_span != LIST_END(&list->items)) {
    LIST_INSERT_BEFORE(next_span, span, freelist);
  } else {
    LIST_INSERT_HEAD(&list->items, span, freelist);
  }

  pthread_mutex_unlock(&list->mutex);
  return 0;
}

static void freelist_reset(struct vp_freelist *list) {
  pthread_mutex_lock(&list->mutex);

  struct vp_span *span, *tmp;
  LIST_FOREACH_SAFE(span, &list->items, freelist, tmp) {
    LIST_REMOVE(span, freelist);
    FREE(span);
  }

  list->nallocs = 0;

  pthread_mutex_unlock(&list->mutex);
}

void *vp_alloc(size_t npages) {
  return freelist_alloc(&g_freelist_main, npages);
}

int vp_free(void *p, size_t npages) {
  return freelist_free(&g_freelist_main, p, npages);
}

void vp_reset(void) {
  freelist_reset(&g_freelist_main);
}
