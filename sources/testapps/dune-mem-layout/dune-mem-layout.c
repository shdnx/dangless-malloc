#include <dangless/virtmem.h>
#include <dangless/dump.h>

#include <dune.h>

#include <stdio.h>

// libdune/entry.c, __setup_mappings_full():
// - first 4 GB: identity-mapped
// - layout->base_map for GPA_MAP_SIZE: dune_mmap_addr_to_pa() mapped
// - layout->base_stack for GPA_STACK_SIZE
// - VDSO and VVAR
// - VSYSCALL (0xffffffffff600000)

#include "virtmem_chart_layout.c"

int main(int argc, const char *argv[]) {
  fprintf(stderr, "\nDune values:\n");
  fprintf(stderr, "phys_limit = %p\n", (void *)phys_limit);
  fprintf(stderr, "mmap_base = %p, GPA_MAP_SIZE = 0x%lx, so ends at: %p\n", (void *)mmap_base, GPA_MAP_SIZE, (char *)mmap_base + GPA_MAP_SIZE);
  fprintf(stderr, "stack_base = %p, GPA_STACK_SIZE = 0x%lx, so ends at: %p\n", (void *)stack_base, GPA_STACK_SIZE, (char *)stack_base + GPA_STACK_SIZE);
  fprintf(stderr, "PAGEBASE = %p, MAX_PAGES = 0x%lx\n", (void *)PAGEBASE, MAX_PAGES);

  fprintf(stderr, "\n--------\n\n");

  pte_t *pml4 = pt_root();

  fprintf(stderr, "PML4 summary:\n");
  dump_pt_summary(stderr, pml4, PT_L4);

  fprintf(stderr, "\n--------\n\n");

  /*fprintf(stderr, "PML4 dump:\n");
  dump_pt(stderr, pml4, PT_L4);*/

  /*fprintf(stderr, "PML4[0] dump:\n");
  pte_t *pml4_1 = pt_paddr2vaddr(pml4[0] & PTE_FRAME);
  dump_pt_summary(stderr, pml4_1, PT_L3);*/

  struct virtmem_region *region_list = virtmem_chart_layout();
  size_t total_len = 0;
  while (region_list) {
    const bool is_mapped = FLAG_ISSET(region_list->mapped_flags, PTE_V);

    total_len += region_list->length;

    if (is_mapped) {
      fprintf(
        stderr,
        "VA 0x%012lx - 0x%012lx => PA 0x%010lx - 0x%010lx (%zu pages) [",
        region_list->va_start,
        region_list->va_start + region_list->length,
        region_list->pa_start,
        region_list->pa_start + region_list->length,
        region_list->length / PGSIZE
      );

      if (FLAG_ISSET(region_list->mapped_flags, PTE_U)) {
        fprintf(stderr, " user");
      } else {
        fprintf(stderr, " kernel");
      }

      if (FLAG_ISSET(region_list->mapped_flags, PTE_W)) {
        fprintf(stderr, " writeable");
      } else {
        fprintf(stderr, " read-only");
      }

      if (FLAG_ISSET(region_list->mapped_flags, PTE_NX)) {
        fprintf(stderr, " non-executable");
      } else {
        fprintf(stderr, " executable");
      }

      fprintf(stderr, " ]\n");
    }

    region_list = region_list->next_region;
  }

  fprintf(stderr, "Finished, total_len = 0x%lx\n", total_len);

  return 0;
}
