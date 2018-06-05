#include "dangless/common.h"
#include "dangless/config.h"
#include "dangless/virtmem.h"
#include "dangless/virtmem_alloc.h"
#include "dangless/platform/mem.h"
#include "dangless/platform/virtual_remap.h"

#include "dune.h"

#if DANGLESS_CONFIG_DEBUG_VIRTREMAP
  #define LOG(...) vdprintf(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

int vremap_errno = 0;

#define RETURN(CODE) return vremap_errno = (CODE)

// TODO: this is basically the same code as for rumprun (except this is now better, because it supports size > PGSIZE), except for dune_va_to_pa() call - refactor
int vremap_map(void *ptr, size_t size, OUT void **remapped_ptr) {
  ASSERT0(ptr && size);
  ASSERT(in_kernel_mode(), "Virtual remapping is impossible outside of kernel mode!");

  // have to take into account the offset of the data inside the 4K page, since we're going to be allocating virtual 4K pages
  const size_t inpage_offset = get_page_offset((uintptr_t)ptr, PT_L1);
  const size_t npages = ROUND_UP(inpage_offset + size, PGSIZE) / PGSIZE;

  // this assumes that the physical memory backing 'ptr' is contiguous and predictable
  paddr_t pa = dune_va_to_pa(ptr);
  paddr_t pa_page = ROUND_DOWN(pa, PGSIZE);

  LOG("inpage_offset = %zu, npages = %zu, pa = 0x%lx, pa_page = 0x%lx\n", inpage_offset, npages, pa, pa_page);

#if DANGLESS_CONFIG_DEBUG_VIRTREMAP
  // verify that 'ptr' is backed by contigous physical memory in the expected way (regardless of whether it's mapped with hugepages or not)
  {
    enum pt_level base_level;
    paddr_t base_pa_page = pt_resolve_page(ptr, OUT &base_level);
    ASSERT(base_pa_page, "attempted to remap ptr %p that has no physical memory backing!", ptr);

    paddr_t base_pa = base_pa_page + get_page_offset((uintptr_t)ptr, base_level);
    ASSERT(base_pa == pa, "attempted to remap ptr %p that is not mapped predictably: expected PA = 0x%lx, actual PA = 0x%lx (level %u)", ptr, pa, base_pa, base_level);

    const size_t npages_per_mapping = pt_num_mapped_pages(base_level);

    size_t i;
    for (i = npages_per_mapping; i < npages; i += npages_per_mapping) {
      enum pt_level actual_level;
      paddr_t actual_pa = pt_resolve_page(PG_OFFSET(ptr, i), OUT &actual_level);

      paddr_t expected_pa = PG_OFFSET(base_pa_page, i);
      ASSERT(
        actual_level == base_level && expected_pa == actual_pa,
        "attempted to remap ptr %p whose physical memory backing is not contiguous: at page offset %zu, expected PA 0x%lx (level %u), actual PA 0x%lx (level %u)!",
        ptr, i, expected_pa, base_level, actual_pa, actual_level);
    }
  }
#endif

  void *va = vp_alloc(npages);
  if (!va) {
    LOG("could not allocate virtual memory region of %zu pages to remap to\n", npages);
    RETURN(EVREM_NO_VM);
  }

  LOG("got va to remap to: %p\n", va);

#if DANGLESS_CONFIG_DEBUG_VIRTREMAP
  // verify that the allocated virtual memory region is not yet backed by any physical memory
  {
    size_t i;
    for (i = 0; i < npages; i++) {
      enum pt_level level;
      paddr_t pa = pt_resolve_page(PG_OFFSET(va, i), OUT &level);

      ASSERT(!pa, "Allocated virtual page %p (page offset %zu) is already mapped to a physical address (0x%lx at level %u)!", PG_OFFSET(va, i), i, pa, level);
    }
  }
#endif

  // this assumes that 'ptr' is backed by a contiguous physical memory region
  int result;
  if ((result = pt_map_region(pa_page, (vaddr_t)va, npages * PGSIZE, PTE_W | PTE_NX)) < 0) {
    LOG("could not remap pa 0x%lx to va %p, code %d\n", pa_page, va, result);

    // try to give back the virtual memory page - this may fail, but we can't do anything about it
    vp_free(va, npages);
    RETURN(EVREM_VIRT_MAP);
  }

  OUT *remapped_ptr = (u8 *)va + inpage_offset;
  RETURN(VREM_OK);
}

int vremap_resolve(void *ptr, OUT void **original_ptr) {
  ASSERT0(in_kernel_mode());

  // TODO: detect if ptr points to the stack or to a global variable (.bss or .data)

  // Detecting whether a virtual address was remapped:
  // - Do a pagewalk on the virtual address (VA) and get the corresponding physical address (PA).
  // - An address is remapped if dune_va_to_pa(VA) != PA.
  paddr_t pa = pt_resolve(ptr);
  if (!pa) {
    // not backed by physical memory
    RETURN(EVREM_NO_PHYS_BACK);
  }

  if (dune_va_to_pa(ptr) == pa) {
    // not a remapped adddress
    OUT *original_ptr = ptr;
    RETURN(VREM_NOT_REMAPPED);
  }

  // Getting the original (before remapping) virtual address:
  // - The lowest physical address that mmap() yields is phys_limit - GPA_STACK_SIZE - GPA_MAP_SIZE = 512 GB - 1 GB - 63 GB = 448 GB.
  // Anything above this is mmap()-ed address, and we can get back the original virtual address by reversing the operations of dune_mmap_addr_to_pa(). (That is, up until phys_limit - GPA_STACK_SIZE = 511 GB, which is the lowest physical address for the stack.)
  // Anything below this is can be assumed to be identity-mapped, i.e. original VA = PA.
  // This happens to be the same logic we use to determine the virtual address of page table physical addresses. Ugly.
  // TODO: refactor/rename?
  OUT *original_ptr = pt_paddr2vaddr(pa);
  RETURN(VREM_OK);
}

static const char *const g_error_messages[] = {
  [VREM_OK] = "No error",
  [VREM_NOT_REMAPPED] = "Pointer is not a remapped one",
  [_VREM_MAX + -EVREM_NO_VM] = "Out of virtual memory",
  [_VREM_MAX + -EVREM_VIRT_MAP] = "Failed to map virtual memory",
  [_VREM_MAX + -EVREM_NO_PHYS_BACK] = "Pointer is not backed by physical memory"
};

const char *vremap_describe_error(int code) {
  if (_VREM_MIN < code || _VREM_MAX < code)
    return "Invalid result code";

  return code < 0
    ? g_error_messages[-code]
    : g_error_messages[code];
}
