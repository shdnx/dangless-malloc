#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "common.h"
#include "safemalloc.h"
#include "virtmem.h"

#include <sys/queue.h>

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
  if (!span)
    return false;

  span->begin = start;
  span->npages = max_pages;
  SLIST_INSERT_HEAD(&span_list, span, next);

  init = true;
  return true;
}

static vaddr_t alloc_virtual_page(void) {
  if (SLIST_EMPTY(&span_list)) {
    if (!alloc_virtual_mem()) {
      printf("Warning: out of virtual memory!\n");
      return 0;
    }
  }

  struct vmem_page_span *span = SLIST_FIRST(&span_list);
  assert(span);

  vaddr_t va = span->begin;
  span->begin += PAGE_SIZE;
  span->npages--;

  if (span->npages == 0) {
    SLIST_REMOVE_HEAD(&span_list, next);
    free(span);
  }

  return va;
}

void *safe_malloc(size_t sz) {
  void *p = malloc(sz);
  if (!p)
    return p;

  vaddr_t va = alloc_virtual_page();
  if (!va)
    return p;

  if (pt_map(ROUND_DOWN((paddr_t)p, PAGE_SIZE), va, PG_RW | PG_NX) < 0)
    return p;

  return (uint8_t *)va + get_page_offset((uintptr_t)p, PT_4K);
}

void safe_free(void *p) {
  pte_t *ppte;
  enum pt_level level = pt_walk(p, PGWALK_FULL, OUT &ppte);
  assert(FLAG_ISSET(*ppte, PG_V));

  if (level == PT_L1) {
    paddr_t pa = (paddr_t)(*ppte & PG_FRAME);
    *ppte = 0xDEAD00;

    // since the original virtual address == physical address, we can just get the physical address and give that to free()
    // this allows us to get away with not maintaining mappings from the remapped virtual addresses to the original ones
    free((void *)pa);
  } else {
    // we must have failed to allocate a dedicated virtual page, just forward to the standard free()
    free(p);
  }
}