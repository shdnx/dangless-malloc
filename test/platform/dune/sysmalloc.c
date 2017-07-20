#define _GNU_SOURCE
#define SYMBOLS_OVERRIDEN 0

#include <dlfcn.h> // dlsym(), RTLD_NEXT, RTLD_DEFAULT
#include <assert.h>
#include <malloc.h> // malloc_usable_size()

#include "common.h"
#include "platform/mem.h"
#include "platform/sysmalloc.h"

#define SYSMALLOC_DEBUG 1

#if SYSMALLOC_DEBUG
  #define DPRINTF(...) vdprintf(__VA_ARGS__)
#else
  #define DPRINTF(...) /* empty */
#endif

static void*(*addr_sysmalloc)(size_t) = NULL;
static void*(*addr_syscalloc)(size_t, size_t) = NULL;
static void*(*addr_sysrealloc)(void *, size_t) = NULL;
static int(*addr_sysmemalign)(void **, size_t, size_t) = NULL;
static void(*addr_sysfree)(void *) = NULL;

static void populate_addrs() {
  void *h;

#if SYMBOLS_OVERRIDEN
  h = RTLD_NEXT;
#else
  h = RTLD_DEFAULT;
#endif

  addr_sysmalloc = dlsym(h, "malloc");
  addr_syscalloc = dlsym(h, "calloc");
  addr_sysrealloc = dlsym(h, "realloc");
  addr_sysmemalign = dlsym(h, "memalign");
  addr_sysfree = dlsym(h, "free");
}

void *sysmalloc(size_t sz) {
  if (!addr_sysmalloc)
    populate_addrs();

  return addr_sysmalloc(sz);
}

void *syscalloc(size_t num, size_t size) {
  if (!addr_syscalloc)
    populate_addrs();

  return addr_syscalloc(num, size);
}

void *sysrealloc(void *p, size_t sz) {
  if (!addr_sysrealloc)
    populate_addrs();

  return addr_sysrealloc(p, sz);
}

int sysmemalign(void **pp, size_t align, size_t sz) {
  if (!addr_sysmemalign)
    populate_addrs();

  return addr_sysmemalign(pp, align, sz);
}

void sysfree(void *p) {
  if (!addr_sysfree)
    populate_addrs();

  return addr_sysfree(p);
}

size_t sysmalloc_usable_pages(void *p) {
  size_t sz = malloc_usable_size(p);
  return ROUND_UP(sz, PGSIZE) / PGSIZE;
}