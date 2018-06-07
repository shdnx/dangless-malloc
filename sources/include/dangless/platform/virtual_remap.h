#ifndef DANGLESS_PLATFORM_VIRTUAL_REMAP_H
#define DANGLESS_PLATFORM_VIRTUAL_REMAP_H

#include "dangless/common.h"
#include "dangless/platform/mem.h"

enum {
  VREM_OK = 0,

  // given pointer is not remapped
  VREM_NOT_REMAPPED = 1,

  // out of virtual memory
  EVREM_NO_VM = -1,

  // failed to map virtual memory
  EVREM_VIRT_MAP = -2,

  // cannot resolve, because there's no physical memory backing
  EVREM_NO_PHYS_BACK = -3,

  _VREM_MIN = EVREM_NO_PHYS_BACK,
  _VREM_MAX = VREM_NOT_REMAPPED
};

extern int vremap_result;

const char *vremap_get_diag(int code);

static inline const char *vremap_diag(void) {
  return vremap_get_diag(vremap_result);
}

// Given a pointer to a valid allocation and its size, re-maps the backing physical memory into a new virtual memory region, provided by the virtual memory allocator.
// If successful, the 'remapped' out parameter is filled with the new virtual address and the function returns 0. The original pointer remains valid.
int vremap_map(void *ptr, size_t size, OUT void **remapped);

// Given a remapped virtual memory address, attempts to determine the original virtual address. If successful, the 'original_ptr' out parameter is filled with the original virtual address and the function returns 0.
int vremap_resolve(void *ptr, OUT void **original_ptr);

#endif
