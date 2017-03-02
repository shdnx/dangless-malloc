#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <bmk-core/queue.h>

#include "mem.h"
#include "virtmem_alloc.h"

#define VMALLOC_DEBUG 1

#if VMALLOC_DEBUG
  #define VMALLOC_DPRINTF(...) dprintf("[vmalloc] " __VA_ARGS__)
#else
  #define VMALLOC_DPRINTF(...) /* empty */
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

void *vp_alloc(size_t npages) {
  struct vp_span *span;
  LIST_FOREACH(span, &vp_freelist, freelist) {
    if (span_pages(span) >= npages)
      break;
  }

  if (span == LIST_END(&vp_freelist)) {
    VMALLOC_DPRINTF("vp_alloc: could not allocate %zu pages: freelist empty\n", npages);
    return NULL;
  }

  assert(span);
  vaddr_t va = span->start;
  span->start += npages * PGSIZE;

  VMALLOC_DPRINTF("vp_alloc: satisfying %zu page alloc from span %p => 0x%lx\n", npages, span, va);

  if (span_empty(span)) {
    VMALLOC_DPRINTF("vp_alloc: span %p is now empty, deallocating\n", span);
    LIST_REMOVE(span, freelist);
    free(span);
  }

  return (void *)va;
}

int vp_free(void *p, size_t npages) {
  assert((vaddr_t)p % PGSIZE == 0);
  assert(npages > 0);

  vaddr_t start = (vaddr_t)p;
  vaddr_t end = start + npages * PGSIZE;

  // TODO: try merging with other free spans

  struct vp_span *span = malloc(sizeof(struct vp_span));
  if (!span) {
    VMALLOC_DPRINTF("vp_free: could not allocate vp_span: out of memory?\n");
    return -1;
  }

  span->start = (vaddr_t)p;
  span->end = span->start + npages * PGSIZE;

  VMALLOC_DPRINTF("vp_free: new span %p: 0x%lx - 0x%lx (%zu pages)\n", span, span->start, span->end, span_pages(span));

  // insert the new span to the head of the freelist: we want to re-use previously used locations as soon as possible, since we won't need to allocate new page tables
  LIST_INSERT_HEAD(&vp_freelist, span, freelist);
  return 0;
}