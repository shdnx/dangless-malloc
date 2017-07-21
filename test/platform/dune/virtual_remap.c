#include <assert.h>

#include "common.h"
#include "virtmem.h"
#include "virtmem_alloc.h"

#include "platform/mem.h"
#include "platform/virtual_remap.h"

#include "dune.h"

#define VIRTREMAP_DEBUG 1

#if VIRTREMAP_DEBUG
  #define DPRINTF(...) vdprintf(__VA_ARGS__)
#else
  #define DPRINTF(...) /* empty */
#endif

enum {
  VREM_OK = 0,

  // given pointer is not remapped
  VREM_NOT_REMAPPED = 1,

  // out of virtual memory
  EVREM_NO_VM = -1,

  // failed to map virtual memory
  EVREM_VIRT_MAP = -2,

  // cannot resolve, because there's no physical memory backing
  EVREM_NO_PHYS_BACK = -3
};

// We have to know when the first remap has happened. There's usually quite a lot of malloc() action going on before the application even really starts, i.e. even before we enter Dune mode. Before entering Dune mode, vremap_resolve() cannot do pt_resolve(), because that needs access to cr3 that we do not have. The assumption is that dangless_dedicate_vmem() is called only once it's safe to do remapping action, i.e. we're inside Dune mode.
static bool g_has_remap_occured = false;

// TODO: this is basically the same code as for rumprunm, except for dune_va_to_pa() call - refactor
// TODO: this only supports <= PAGESIZE allocation
int vremap_map(void *ptr, size_t size, OUT void **remapped_ptr) {
  assert(ptr);

  void *va = vp_alloc_one();
  if (!va) {
    DPRINTF("could not allocate virtual memory page to remap to\n");
    return EVREM_NO_VM;
  }

#if VIRTREMAP_DEBUG
  {
    enum pt_level level;
    paddr_t pa = pt_resolve_page(va, OUT &level);
    assert(!pa && "Allocated virtual page is already mapped to a physical address!");
  }
#endif

  // this assumes that the physical memory backing 'ptr' is contiguous
  paddr_t pa = dune_va_to_pa(ptr);
  paddr_t pa_page = ROUND_DOWN(pa, PGSIZE);

  // this assumes that the allocated virtual memory region is backed by a contiguous physical memory region
  int result;
  if ((result = pt_map_region(pa_page, (vaddr_t)va, ROUND_UP(size, PGSIZE), PTE_W | PTE_NX)) < 0) {
    DPRINTF("could not remap pa 0x%lx to va %p, code %d\n", pa_page, va, result);

    // try to give back the virtual memory page - this may fail, but we can't do anything about it
    vp_free_one(va);
    return EVREM_VIRT_MAP;
  }

  OUT *remapped_ptr = (uint8_t *)va + get_page_offset((uintptr_t)ptr, PT_4K);
  g_has_remap_occured = true;
  return VREM_OK;
}

int vremap_resolve(void *ptr, OUT void **original_ptr) {
  if (!g_has_remap_occured) {
    // If no remapping has happened as of yet, it's both pointless and dangerous to do to pt_resolve(): see the comments at g_has_remap_occured.
    OUT *original_ptr = ptr;
    return VREM_NOT_REMAPPED;
  }

  // Detecting whether a virtual address was remapped:
  // - Do a pagewalk on the virtual address (VA) and get the corresponding physical address (PA).
  // - An address is remapped if dune_va_to_pa(VA) != PA.
  paddr_t pa = pt_resolve(ptr);
  if (!pa) {
    // not backed by physical memory
    return EVREM_NO_PHYS_BACK;
  }

  if (dune_va_to_pa(ptr) == pa) {
    // not a remapped adddress
    OUT *original_ptr = ptr;
    return VREM_NOT_REMAPPED;
  }

  // Getting the original (before remapping) virtual address:
  // - The lowest physical address that mmap() yields is phys_limit - GPA_STACK_SIZE - GPA_MAP_SIZE = 512 GB - 1 GB - 63 GB = 448 GB.
  // Anything above this is mmap()-ed address, and we can get back the original virtual address by reversing the operations of dune_mmap_addr_to_pa(). (That is, up until phys_limit - GPA_STACK_SIZE = 511 GB, which is the lowest physical address for the stack.)
  // Anything below this is can be assumed to be identity-mapped, i.e. original VA = PA.
  // This happens to be the same logic we use to determine the virtual address of page table physical addresses. Ugly.
  // TODO: refactor/rename?
  OUT *original_ptr = pt_paddr2vaddr(pa);
  return VREM_OK;
}
