#include <assert.h>

#include "common.h"
#include "virtmem.h"
#include "virtmem_alloc.h"
#include "platform/virtual_remap.h"

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
  EVREM_VIRT_MAP = -2
};

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

  // this assumes that an identity-mapping exists between the physical and virtual memory
  // it further assumes that the physical memory backing 'ptr' is contiguous
  paddr_t pa = (paddr_t)ptr;
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
  return VREM_OK;
}

int vremap_resolve(void *ptr, OUT void **original_ptr) {
  pte_t *ppte;
  enum pt_level level = pt_walk(ptr, PGWALK_FULL, OUT &ppte);
  assert(FLAG_ISSET(*ppte, PTE_V));

  // TODO: use instead one of the available PTE bits to note the fact that a page is dangless-remapped (unless somebody already uses those bits?)
  // currently this is only possible when we failed to allocate a dedicated virtual page for this allocation, and just falled back to proxying sysmalloc()
  if (level != PT_L1) {
    OUT *original_ptr = ptr;
    return VREM_NOT_REMAPPED;
  }

  // since the original virtual address == physical address, we can just get the physical address and pretend it's a virtual address: specifically, it's the original virtual address, before we remapped it with virtual_remap()
  // this trickery allows us to get away with not maintaining mappings from the remapped virtual addresses to the original ones
  paddr_t pa = (paddr_t)(*ppte & PTE_FRAME_L1) + get_page_offset((uintptr_t)ptr, PT_L1);
  OUT *original_ptr = (void *)pa;
  return VREM_OK;
}