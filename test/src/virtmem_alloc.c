#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "queue.h"
#include "virtmem_alloc.h"

#include "platform/mem.h"
#include "platform/sysmalloc.h"

#define VMALLOC_DEBUG 1

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

static inline size_t span_pages(struct vp_span *span) {
  return (span->end - span->start) / PGSIZE;
}

static inline bool span_empty(struct vp_span *span) {
  return span->start == span->end;
}

static LIST_HEAD(, vp_span) vp_freelist = LIST_HEAD_INITIALIZER(&vp_freelist);

// TODO: is this needed? simple compare-and-exchange? how about span merging?
//static LIST_HEAD(, vp_span) vp_freelist2;

//static pthread_mutex_t vp_migration_mutex = PTHREAD_MUTEX_INITIALIZER;

void *vp_alloc(size_t npages) {
  struct vp_span *span;
  LIST_FOREACH(span, &vp_freelist, freelist) {
    if (span_pages(span) >= npages)
      break;
  }

  if (UNLIKELY(span == LIST_END(&vp_freelist))) {
    DPRINTF("vp_alloc: could not allocate %zu pages: freelist empty\n", npages);
    return NULL;
  }

  ASSERT0(span);
  vaddr_t va = span->start;
  span->start += npages * PGSIZE;

  DPRINTF("vp_alloc: satisfying %zu page alloc from span %p => 0x%lx\n", npages, span, va);

  if (span_empty(span)) {
    DPRINTF("vp_alloc: span %p is now empty, deallocating\n", span);
    LIST_REMOVE(span, freelist);
    FREE(span);
  }

  return (void *)va;
}

int vp_free(void *p, size_t npages) {
  ASSERT0((vaddr_t)p % PGSIZE == 0);
  ASSERT0(npages > 0);

  vaddr_t start = (vaddr_t)p;
  vaddr_t end = start + npages * PGSIZE;

  // TODO: try merging with other free spans

  struct vp_span *span = MALLOC(struct vp_span);
  if (UNLIKELY(!span)) {
    DPRINTF("vp_free: could not allocate vp_span: out of memory?\n");
    return -1;
  }

  span->start = (vaddr_t)p;
  span->end = span->start + npages * PGSIZE;

  DPRINTF("vp_free: new span %p: 0x%lx - 0x%lx (%zu pages)\n", span, span->start, span->end, span_pages(span));

  // insert the new span to the head of the freelist: we want to re-use previously used locations as soon as possible, since we won't need to allocate new page tables
  LIST_INSERT_HEAD(&vp_freelist, span, freelist);
  return 0;
}
