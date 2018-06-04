#define _GNU_SOURCE

#include <stdlib.h> // abort()
#include <dlfcn.h> // dlsym(), RTLD_NEXT, RTLD_DEFAULT
#include <malloc.h> // malloc_usable_size()

#include "dangless/common.h"
#include "dangless/config.h"
#include "dangless/platform/mem.h"
#include "dangless/platform/sysmalloc.h"

#if DANGLESS_CONFIG_DEBUG_SYSMALLOC
  #define LOG(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
#endif

static bool g_populating = false;

static void*(*addr_sysmalloc)(size_t) = NULL;
static void*(*addr_syscalloc)(size_t, size_t) = NULL;
static void*(*addr_sysrealloc)(void *, size_t) = NULL;
static int(*addr_sysmemalign)(void **, size_t, size_t) = NULL;
static void(*addr_sysfree)(void *) = NULL;

// This function will only run once, during the first allocation. Since that will happen well before main(), there's no need for any syncronization around here.
static void populate_addrs(void) {
  ASSERT(!g_populating, "Recursive populate_addrs() call!");
  g_populating = true;

  void *h;

#if DANGLESS_CONFIG_OVERRIDE_SYMBOLS
  h = RTLD_NEXT;
#else
  h = RTLD_DEFAULT;
#endif

  char *err;

#define HANDLE_SYM(FN) \
  do { \
    addr_sys##FN = dlsym(h, #FN); \
    /* Weird: if I use ASSERT(!(err = dlerror()), ...) then weird things happen in release mode; so have to check for this separately */ \
    if ((err = dlerror())) { \
      vdprintf_nomalloc("Could not get symbol address for %s: dlerror: %s", #FN, err); \
      abort(); \
    } \
  } while (0)

  HANDLE_SYM(malloc);
  HANDLE_SYM(calloc);
  HANDLE_SYM(realloc);
  HANDLE_SYM(memalign);
  HANDLE_SYM(free);

#undef HANDLE_SYM

  g_populating = false;
}

void *sysmalloc(size_t sz) {
  if (UNLIKELY(!addr_sysmalloc))
    populate_addrs();

  return addr_sysmalloc(sz);
}

static void *syscalloc_special(size_t num, size_t size) {
  static char s_storage[DANGLESS_CONFIG_CALLOC_SPECIAL_BUFSIZE];
  static bool s_storage_used = false;

  ASSERT(!s_storage_used, "Attempted to use the syscalloc special allocator again!");
  s_storage_used = true;

  // TODO: alignment?
  const size_t requested_size = num * size;
  ASSERT(sizeof(s_storage) >= requested_size, "Insufficient special calloc storage (%zu) to satisfy request (%zu * %zu = %zu)!!!", sizeof(s_storage), num, size, requested_size);

  LOG("Allocating %zu bytes from syscalloc special storage (capacity %zu)\n", requested_size, sizeof(s_storage));
  return s_storage;
}

void *syscalloc(size_t num, size_t size) {
  if (UNLIKELY(!addr_syscalloc)) {
    if (g_populating) {
      // This is only possible when dlsym() was called for the first time, i.e. upon the very first allocation: it calls calloc(). We obviously cannot call populate_addrs(), because that'd cause an infinite recursion. This calloc() has to be implemented manually.
      // Since the first memory allocation happens well before main(), we don't have to bother with multi-threading here (thank god for that, it'd be awkward).
      // Sources:
      //  - https://github.com/lattera/glibc/blob/59ba27a63ada3f46b71ec99a314dfac5a38ad6d2/dlfcn/dlerror.c#L141
      //  - https://www.slideshare.net/tetsu.koba/tips-of-malloc-free (slide 34)
      //  - https://github.com/ugtrain/ugtrain/blob/d9519a15f4363cee48bb3c270f6050181c5aa305/src/linuxhooking/memhooks.c#L153

      LOG("populate_addrs() is already running: special dlsym() calloc() handling active!\n");

      return syscalloc_special(num, size);
    }

    populate_addrs();
  }

  return addr_syscalloc(num, size);
}

void *sysrealloc(void *p, size_t sz) {
  if (UNLIKELY(!addr_sysrealloc))
    populate_addrs();

  return addr_sysrealloc(p, sz);
}

int sysmemalign(void **pp, size_t align, size_t sz) {
  if (UNLIKELY(!addr_sysmemalign))
    populate_addrs();

  return addr_sysmemalign(pp, align, sz);
}

void sysfree(void *p) {
  if (UNLIKELY(!addr_sysfree))
    populate_addrs();

  return addr_sysfree(p);
}

size_t sysmalloc_usable_pages(void *p) {
  const size_t sz = malloc_usable_size(p);
  return ROUND_UP(sz, PGSIZE) / PGSIZE;
}
