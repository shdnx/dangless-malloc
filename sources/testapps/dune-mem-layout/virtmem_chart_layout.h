#ifndef DANGLESS_VIRTMEM_CHART_LAYOUT_H
#define DANGLESS_VIRTMEM_CHART_LAYOUT_H

#include "dangless/platform/mem.h"

#include "dangless/virtmem.h"

#include "dangless/common/types.h"

struct virtmem_region {
  vaddr_t va_start;
  paddr_t pa_start;
  size_t length;
  enum pte_flags mapped_flags;

  struct virtmem_region *next_region;
};

struct virtmem_region *virtmem_region_alloc(void);

void virtmem_region_free(struct virtmem_region *);

void virtmem_region_free_list(struct virtmem_region *list);

struct virtmem_region *virtmem_chart_layout(void);

#endif
