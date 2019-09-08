#include "virtmem_chart_layout.h"

#include <dangless/platform/sysmalloc.h>
#include <dangless/virtmem.h>
#include <dangless/common/assert.h>

#include <dune.h>

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>

// libdune/entry.c, __setup_mappings_full():
// - first 4 GB: identity-mapped
// - layout->base_map for GPA_MAP_SIZE: dune_mmap_addr_to_pa() mapped
// - layout->base_stack for GPA_STACK_SIZE
// - VDSO and VVAR
// - VSYSCALL (0xffffffffff600000)

static bool check_if_region_contains(
  const char *what,
  const void *ptr,
  const struct virtmem_region *region
) {
  const vaddr_t addr = (vaddr_t)ptr;

  if (region->va_start <= addr && addr < region->va_start + region->length) {
    printf("[%s] ", what);
    return true;
  }

  return false;
}

static void print_dune_values(void) {
  printf("phys_limit = %p\n", (void *)phys_limit);
  printf("mmap_base = %p, GPA_MAP_SIZE = 0x%lx, so ends at: %p\n", (void *)mmap_base, GPA_MAP_SIZE, (char *)mmap_base + GPA_MAP_SIZE);
  printf("stack_base = %p, GPA_STACK_SIZE = 0x%lx, so ends at: %p\n", (void *)stack_base, GPA_STACK_SIZE, (char *)stack_base + GPA_STACK_SIZE);
  printf("PAGEBASE = %p, MAX_PAGES = 0x%lx\n", (void *)PAGEBASE, MAX_PAGES);
}

static void print_host_procmap(void) {
  FILE *fp = fopen("/proc/self/maps", "r");
  if (fp) {
    char fileread_buffer[1024];

    size_t read;
    while ((read = fread(fileread_buffer, sizeof(char), sizeof(fileread_buffer), fp)) > 0) {
       puts(fileread_buffer);
    }

    if (!feof(fp)) {
      perror("error interrupted reading file");
    }

    fclose(fp);
  } else {
    perror("failed to open file");
  }
}

static void print_virtmem_region(FILE *os, const struct virtmem_region *region) {
  fprintf(os, "VA " VADDR_FMT " - " VADDR_FMT, region->va_start, region->va_start + region->length);

  const bool is_mapped = FLAG_ISSET(region->mapped_flags, PTE_V);

  if (is_mapped) {
    fprintf(os, " => PA " PADDR_FMT " - " PADDR_FMT " [", region->pa_start, region->pa_start + region->length);

    if (FLAG_ISSET(region->mapped_flags, PTE_U)) {
      fprintf(os, " user");
    } else {
      fprintf(os, " kernel");
    }

    if (FLAG_ISSET(region->mapped_flags, PTE_W)) {
      fprintf(os, " write");
    } else {
      fprintf(os, " readonly");
    }

    if (FLAG_ISSET(region->mapped_flags, PTE_NX)) {
      fprintf(os, " no-exec");
    } else {
      fprintf(os, " exec");
    }

    fprintf(os, " ]\n");
  } else {
    fprintf(os, ": unmapped\n");
  }
}

int g_dummy = 432;

int main(int argc, const char *argv[]) {
  int dummy_var = argc * 2;

  void *ptr_stack = &dummy_var;
  void *ptr_data = &g_dummy;
  void *ptr_code = &virtmem_region_free_list;

  int *ptr_heap_sysmalloc = MALLOC(int);
  if (!ptr_heap_sysmalloc) {
    perror("sysmalloc() failed!");
    return -1;
  }

  int *ptr_heap_dglmalloc = malloc(sizeof(int));
  if (!ptr_heap_dglmalloc) {
    perror("dangless_malloc() failed!");
    return -2;
  }

  void *ptr_heap_mmap = mmap(NULL, /*length=*/4096, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  if (ptr_heap_mmap == MAP_FAILED) {
    perror("mmap() failed");
    return -3;
  }

  printf("\n-------- Dune values --------\n\n");

  print_dune_values();

  printf("\n-------- /proc/self/maps --------\n\n");

  print_host_procmap();

  printf("\n-------- virtual memory layout --------\n\n");

  size_t total_len = 0;

  bool found_stack_region = false;
  bool found_data_region = false;
  bool found_heap_sysmalloc_region = false;
  bool found_heap_dglmalloc_region = false;
  bool found_heap_mmap_region = false;
  bool found_code_region = false;

  struct virtmem_region *region;
  for (region = virtmem_chart_layout();
       region != NULL;
       region = region->next_region) {
    total_len += region->length;

    found_code_region |= check_if_region_contains("code", ptr_code, region);
    found_data_region |= check_if_region_contains("data", ptr_data, region);
    found_stack_region |= check_if_region_contains("stack", ptr_stack, region);
    found_heap_sysmalloc_region |= check_if_region_contains("sysmalloc heap", ptr_heap_sysmalloc, region);
    found_heap_dglmalloc_region |= check_if_region_contains("dangless heap", ptr_heap_dglmalloc, region);
    found_heap_mmap_region |= check_if_region_contains("mmap heap", ptr_heap_mmap, region);

    print_virtmem_region(stdout, region);
  }

  if (!found_stack_region)
    printf("warning: stack region was not found (looking for ptr: %p)\n", ptr_stack);

  if (!found_data_region)
    printf("warning: data region was not found (looking for ptr: %p)\n", ptr_data);

  if (!found_heap_sysmalloc_region)
    printf("warning: sysmalloc heap region was not found (looking for ptr: %p)\n", ptr_heap_sysmalloc);

  if (!found_heap_dglmalloc_region)
    printf("warning: dangless heap region was not found (looking for ptr: %p)\n", ptr_heap_dglmalloc);

  if (!found_heap_mmap_region)
    printf("warning: mmap heap region was not found (looking for ptr: %p)\n", ptr_heap_mmap);

  if (!found_code_region)
    printf("warning: code region was not found (looking for ptr: %p)\n", ptr_code);

  ASSERT(total_len == 0x1000000000000, "virtmem_chart_layout() was incomplete, total region length = 0x%lx, expected 0x1000000000000 (256 TB)", total_len);

  return 0;
}
