#include <pthread.h>

#include "common.h"
#include "config.h"
#include "queue.h"
#include "virtmem_alloc.h"

#include "platform/mem.h"
#include "platform/sysmalloc.h"

#if VMALLOC_DEBUG
  #define DPRINTF(...) vdprintf(__VA_ARGS__)
#else
  #define DPRINTF(...) /* empty */
#endif

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
  LIST_HEAD(, vp_span) items;
  pthread_mutex_t mutex;

  // only used if VMALLOC_STATS == 1
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
    DPRINTF("could not allocate %zu pages: freelist empty\n", npages);
    pthread_mutex_unlock(&list->mutex);
    return NULL;
  }

  ASSERT0(span);
  vaddr_t va = span->start;
  span->start += npages * PGSIZE;

#if VMALLOC_STATS
  list->nallocs++;
#endif

  DPRINTF("satisfying %zu page alloc from span %p => 0x%lx\n", npages, span, va);

  if (span_empty(span)) {
    DPRINTF("span %p is now empty, deallocating\n", span);
    LIST_REMOVE(span, freelist);
    FREE(span);
  }

  pthread_mutex_unlock(&list->mutex);
  return (void *)va;
}

static int freelist_free(struct vp_freelist *list, void *p, size_t npages) {
  ASSERT0((vaddr_t)p % PGSIZE == 0);
  ASSERT0(npages > 0);

  vaddr_t start = (vaddr_t)p;
  vaddr_t end = PG_OFFSET(start, npages);

  // TODO: try merging with other free spans

  struct vp_span *span = MALLOC(struct vp_span);
  if (UNLIKELY(!span)) {
    DPRINTF("could not allocate vp_span: out of memory?\n");
    return -1;
  }

  span->start = start;
  span->end = end;

  DPRINTF("new span %p: 0x%lx - 0x%lx (%zu pages)\n", span, span->start, span->end, span_num_pages(span));

  pthread_mutex_lock(&list->mutex);

  // insert the new span to the head of the freelist: we want to re-use previously used locations as soon as possible, since we won't need to allocate new page tables
  LIST_INSERT_HEAD(&list->items, span, freelist);

  pthread_mutex_unlock(&list->mutex);
  return 0;
}

void *vp_alloc(size_t npages) {
  return freelist_alloc(&g_freelist_main, npages);
}

int vp_free(void *p, size_t npages) {
  return freelist_free(&g_freelist_main, p, npages);
}
