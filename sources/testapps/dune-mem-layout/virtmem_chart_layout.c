#include "virtmem_chart_layout.h"

#include "dangless/platform/sysmalloc.h"

#include "dangless/common/assert.h"
#include "dangless/common/util.h"
#include "dangless/common/dprintf.h"

#define LOG(...) vdprintf(__VA_ARGS__)

struct virtmem_region *virtmem_region_alloc(void) {
  struct virtmem_region *new_region = MALLOC(struct virtmem_region);
  ASSERT(new_region, "failed to allocate virtmem_region");

  new_region->va_start = 0;
  new_region->pa_start = 0;
  new_region->length = 0;
  new_region->mapped_flags = 0;
  new_region->next_region = NULL;
  return new_region;
}

void virtmem_region_free(struct virtmem_region *region) {
  sysfree(region);
}

void virtmem_region_free_list(struct virtmem_region *list) {
  while (list) {
    struct virtmem_region *next = list->next_region;
    virtmem_region_free(list);
    list = next;
  }
}

static struct virtmem_region *virtmem_region_add_next(struct virtmem_region *current_region) {
  ASSERT0(!current_region->next_region);

  struct virtmem_region *new_region = virtmem_region_alloc();
  new_region->va_start = current_region->va_start + current_region->length;

  current_region->next_region = new_region;
  return new_region;
}

static struct virtmem_region *virtmem_chart_layout_internal(
  struct virtmem_region *current_region,
  pte_t *root_pt,
  enum pt_level root_level
) {
  const unsigned level_shift = pt_level_shift(root_level);
  const size_t pte_mapped_length = (1uL << level_shift);

  const enum pte_flags flag_mask = PTE_V | PTE_W | PTE_U | PTE_NX;

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    const pte_t pte = root_pt[i];

    // TODO: use PTE_FRAME_L2, _L3?
    const paddr_t pte_pa = (paddr_t)(pte & PTE_FRAME);
    const enum pte_flags pte_flags = (enum pte_flags)(pte & flag_mask);
    const bool pte_is_mapped = FLAG_ISSET(pte, PTE_V);

    const bool current_region_is_mapped = FLAG_ISSET(current_region->mapped_flags, PTE_V);
    const paddr_t current_region_pa_end = current_region->pa_start + current_region->length;

    if (root_level > PT_L1) {
      if (pte_is_mapped) {
        const bool is_hugepage = FLAG_ISSET(pte, PTE_PS);

        if (is_hugepage) {
          if (pte_flags == current_region->mapped_flags
              && pte_pa == current_region_pa_end
          ) {
            current_region->length += pte_mapped_length;
          } else {
            current_region = virtmem_region_add_next(current_region);
            current_region->pa_start = pte_pa;
            current_region->length = pte_mapped_length;
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
      } else if (current_region_is_mapped) {
        // the virtual memory region until here is mapped, but here there's a high-level gap

        current_region = virtmem_region_add_next(current_region);
        current_region->length = pte_mapped_length;
        current_region->mapped_flags = pte_flags;
      } else {
        // grow the unmapped region
        current_region->length += pte_mapped_length;
      }
    } else {
      ASSERT0(root_level == PT_L1);
      ASSERT0(pte_mapped_length == PGSIZE);

      if (pte_flags == current_region->mapped_flags
          && (
            !pte_is_mapped
            || pte_pa == current_region_pa_end
          )) {
        current_region->length += PGSIZE;
      } else {
        current_region = virtmem_region_add_next(current_region);
        current_region->pa_start = pte_is_mapped ? pte_pa : (paddr_t)0;
        current_region->length = PGSIZE;
        current_region->mapped_flags = pte_flags;
      }
    }

    ASSERT(current_region->length % PGSIZE == 0,
           "current_region (start va = 0x%lx) has invalid length of 0x%lx, not multiple of PGSIZE!",
           current_region->va_start,
           current_region->length);
  } // end for

  return current_region;
}

struct virtmem_region *virtmem_chart_layout(void) {
  struct virtmem_region *region0 = virtmem_region_alloc();
  virtmem_chart_layout_internal(region0, pt_root(), PT_L4);

  if (region0->length == 0) {
    // the initial region we allocated here with flags = 0 may be immediately replaced if VA 0 is, strangely, mapped, in which case it'll end up being a region of 0 length
    // this does not convey any information, so we skip it and just return the next one
    struct virtmem_region *region1 = region0->next_region;
    virtmem_region_free(region0);
    return region1;
  }

  return region0;
}
