#include <dangless/virtmem.h>
#include <dangless/dump.h>

#include <libdune/dune.h>

#include <stdio.h>

// libdune/entry.c, __setup_mappings_full():
// - first 4 GB: identity-mapped
// - layout->base_map for GPA_MAP_SIZE: dune_mmap_addr_to_pa() mapped
// - layout->base_stack for GPA_STACK_SIZE
// - VDSO and VVAR
// - VSYSCALL (0xffffffffff600000)

int main(int argc, const char *argv[]) {
  pte_t *pml4 = pt_root();

  fprintf(stderr, "PML4 dump:\n");
  dump_pt(stderr, pml4, PT_L4);

  fprintf(stderr, "\nPML4 summary:\n");
  dump_pt_summary(stderr, pml4, PT_L4);

  fprintf(stderr, "\nDune values:\n");
  fprintf(stderr, "phys_limit = %p\n", (void *)phys_limit);
  fprintf(stderr, "mmap_base = %p, GPA_MAP_SIZE = 0x%lx, so ends at: %p\n", (void *)mmap_base, GPA_MAP_SIZE, (char *)mmap_base + GPA_MAP_SIZE);
  fprintf(stderr, "stack_base = %p, GPA_STACK_SIZE = 0x%lx, so ends at: %p\n", (void *)stack_base, GPA_STACK_SIZE, (char *)stack_base + GPA_STACK_SIZE);
  fprintf(stderr, "PAGEBASE = %p, MAX_PAGES = 0x%lx\n", (void *)PAGEBASE, MAX_PAGES);

  return 0;
}
