#include "dangless/common/dprintf.h"

#include "dangless/common/debug.h"

static void do_dump_mem(const void *ptr, size_t len) {
  const u8 *const start = (const u8 *)ptr;
  const u8 *const end = start + len;

  for (const u64 *p = (const u64 *)start; (uintptr_t)p < (uintptr_t)end; p++) {
    dprintf("[%02lx] %p\t", (uintptr_t)p - (uintptr_t)start, (const void *)p);
    dprintf("%lx\n", *p);
  }
}

void dump_var_mem(const char *name, const void *ptr, size_t len) {
  dprintf("MEMORY DUMP OF '%s' (%p with size %zu):\n", name, ptr, len);
  do_dump_mem(ptr, len);
  dprintf("END MEMORY DUMP of '%s'\n\n", name);
}

void dump_mem_region(const void *ptr, size_t len) {
  dprintf("MEMORY DUMP OF %p - %p (length %zu):\n", ptr, (const u8 *)ptr + len, len);
  do_dump_mem(ptr, len);
  dprintf("END MEMORY DUMP\n");
}
