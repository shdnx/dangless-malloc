#include "dangless/common/assert.h"
#include "dangless/common/types.h"
#include "dangless/common/util.h"
#include "dangless/platform/mem.h"
#include "dangless/platform/sysmalloc.h"
#include "dangless/virtmem.h"

#define LOG(...) fprintf(stderr, "[virtmem_chart_layout] " __VA_ARGS__);

struct virtmem_region {
  vaddr_t va_start;
  paddr_t pa_start;
  size_t length;
  enum pte_flags mapped_flags;

  struct virtmem_region *next_region;
};

struct virtmem_region *virtmem_region_alloc(void) {
  struct virtmem_region *new_region = MALLOC(struct virtmem_region);
  ASSERT(new_region, "Failed to allocate virtmem_region");

  new_region->va_start = 0;
  new_region->pa_start = 0;
  new_region->length = 0;
  new_region->mapped_flags = 0;
  new_region->next_region = NULL;
  return new_region;
}

struct virtmem_region *virtmem_region_add_next(struct virtmem_region *current_region) {
  ASSERT0(!current_region->next_region);

  //LOG("finished region: VA start 0x%lx, PA start 0x%lx, length %zu, flags 0x%lx\n", current_region->va_start, current_region->pa_start, current_region->length, current_region->mapped_flags);

  struct virtmem_region *new_region = virtmem_region_alloc();
  new_region->va_start = current_region->va_start + current_region->length;

  current_region->next_region = new_region;
  return new_region;
}

void virtmem_region_free_list(struct virtmem_region *list) {
  while (list) {
    struct virtmem_region *next = list->next_region;
    sysfree(list);
    list = next;
  }
}

struct virtmem_region *virtmem_chart_layout_internal(
  struct virtmem_region *current_region,
  pte_t *root_pt,
  enum pt_level root_level
) {
  const unsigned level_shift = pt_level_shift(root_level);

  const enum pte_flags flag_mask = PTE_V | PTE_W | PTE_U | PTE_NX;

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    const pte_t pte = root_pt[i];
    //LOG("L%u[%zu]: pte = %lx\n", root_level, i, pte);

    const paddr_t pte_pa = (paddr_t)(pte & PTE_FRAME);
    const enum pte_flags pte_flags = (enum pte_flags)(pte & flag_mask);
    const bool pte_is_mapped = FLAG_ISSET(pte, PTE_V);

    if (root_level > PT_L1) {
      //LOG("L%u[%zu]: pte = %lx (pa = 0x%lx, flags = 0x%lx)\n", root_level, i, pte, pte_pa, pte_flags);

      if (pte_is_mapped) {
        if (FLAG_ISSET(pte, PTE_PS)) {
          // hugepage
          if (pte_flags == current_region->mapped_flags
              && pte_pa == current_region->pa_start + current_region->length
          ) {
            current_region->length += (1u << level_shift);
          } else {
            current_region = virtmem_region_add_next(current_region);
            current_region->pa_start = pte_pa;
            current_region->length = (1u << level_shift);
            current_region->mapped_flags = pte_flags;
          }
        } else {
          // just because a high-level page table exists (except if it's a hugepage mapping), it could be empty, so it doesn't mean anything even if current_region is unmapped
          current_region = virtmem_chart_layout_internal(
            current_region,
            (pte_t *)pt_paddr2vaddr(pte_pa),
            root_level - 1
          );
        }
      } else if (FLAG_ISSET(current_region->mapped_flags, PTE_V)) {
        // the virtual memory region until here is mapped, but here there's a high-level gap
        current_region = virtmem_region_add_next(current_region);

        ASSERT(root_level != PT_L4 || current_region->va_start == (i << level_shift), "Math error with virtual region boundaries: got 0x%lx, expected 0x%lx", current_region->va_start, (i << level_shift));

        current_region->length = 1u << level_shift;
        current_region->mapped_flags = pte_flags;
      } else {
        current_region->length += 1u << level_shift;
      }
    } else {
      ASSERT0(root_level == PT_L1);

      if (pte_flags == current_region->mapped_flags
          && (
            !pte_is_mapped
            || pte_pa == current_region->pa_start + current_region->length
          )) {
        current_region->length += PGSIZE;
      } else {
        current_region = virtmem_region_add_next(current_region);
        current_region->pa_start = pte_is_mapped ? pte_pa : (paddr_t)0;
        current_region->length = PGSIZE;
        current_region->mapped_flags = pte_flags;
      }
    }
  }

  return current_region;
}

struct virtmem_region *virtmem_chart_layout(void) {
  struct virtmem_region *region0 = virtmem_region_alloc();
  virtmem_chart_layout_internal(region0, pt_root(), PT_L4);
  return region0;
}
