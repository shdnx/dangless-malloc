#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "safemalloc.h"
#include "virtmem.h"

#include <bmk-core/queue.h>

#define SAFEMALLOC_DEBUG 1

#if SAFEMALLOC_DEBUG
  #define SAFEMALLOC_DPRINTF(...) dprintf("[safemalloc] " __VA_ARGS__)
#else
  #define SAFEMALLOC_DPRINTF(...) /* empty */
#endif

struct vmem_page_span {
  vaddr_t begin;
  size_t npages;

  SLIST_ENTRY(vmem_page_span) next;
};

static SLIST_HEAD(, vmem_page_span) span_list = SLIST_HEAD_INITIALIZER(vmem_span_list);

static bool alloc_virtual_mem(void) {
  static const vaddr_t start = (vaddr_t)(5 * GB);
  static const size_t max_pages = 10000;
  static bool init = false;

  if (init)
    return false;

  struct vmem_page_span *span = malloc(sizeof(struct vmem_page_span));
  if (!span) {
    SAFEMALLOC_DPRINTF("alloc_virtual_mem failed: malloc returned NULL\n");
    return false;
  }

  span->begin = start;
  span->npages = max_pages;
  SLIST_INSERT_HEAD(&span_list, span, next);

  init = true;
  return true;
}

static vaddr_t alloc_virtual_page(void) {
  if (SLIST_EMPTY(&span_list)) {
    if (!alloc_virtual_mem()) {
      SAFEMALLOC_DPRINTF("alloc_virtual_page: out of virtual memory!\n");
      return 0;
    }
  }

  struct vmem_page_span *span = SLIST_FIRST(&span_list);
  assert(span);

  vaddr_t va = span->begin;

  enum pt_level level;
  paddr_t pa = get_paddr_page((void *)va, OUT &level);
  assert(!pa && "Allocated dirtual page is already mapped to a physical address!");

  span->begin += PGSIZE;
  span->npages--;

  if (span->npages == 0) {
    SLIST_REMOVE_HEAD(&span_list, next);
    free(span);
  }

  return va;
}

void *safe_malloc(size_t sz) {
  void *p = malloc(sz);
  if (!p) {
    SAFEMALLOC_DPRINTF("safe_malloc failed: malloc() returned NULL!\n");
    return NULL;
  }

  vaddr_t va = alloc_virtual_page();
  if (!va) {
    SAFEMALLOC_DPRINTF("safe_malloc failed: alloc_virtual_page() returned NULL!\n");
    return p;
  }

  paddr_t pa = ROUND_DOWN((paddr_t)p, PGSIZE);
  int result;
  if ((result = pt_map(pa, va, PG_RW | PG_NX)) < 0) {
    SAFEMALLOC_DPRINTF("safe_malloc failed: could not map pa 0x%lx to va 0x%lx!, code %d\n", pa, va, result);
    return p;
  }

  return (uint8_t *)va + get_page_offset((uintptr_t)p, PT_4K);
}

void safe_free(void *p) {
  pte_t *ppte;
  enum pt_level level = pt_walk(p, PGWALK_FULL, OUT &ppte);
  assert(FLAG_ISSET(*ppte, PG_V));

  if (level == PT_L1) {
    paddr_t pa = (paddr_t)(*ppte & PG_FRAME);
    pa += get_page_offset((uintptr_t)p, PT_L1);

    *ppte = 0xDEAD00;

    // since the original virtual address == physical address, we can just get the physical address and give that to free()
    // this allows us to get away with not maintaining mappings from the remapped virtual addresses to the original ones
    free((void *)pa);
  } else {
    // we must have failed to allocate a dedicated virtual page, just forward to the standard free()
    free(p);
  }
}