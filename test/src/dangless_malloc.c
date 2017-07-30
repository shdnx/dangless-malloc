#include <pthread.h>

#include "config.h"
#include "dangless_malloc.h"
#include "virtmem.h"
#include "virtmem_alloc.h"

#include "platform/sysmalloc.h"
#include "platform/virtual_remap.h"

#if DGLMALLOC_DEBUG
  #define DPRINTF(...) vdprintf(__VA_ARGS__)
  #define DPRINTF_NOMALLOC(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define DPRINTF(...) /* empty */
  #define DPRINTF_NOMALLOC(...) /* empty */
#endif

enum {
  DEAD_PTE = 0xDEAD00 // the PG_V bit must be 0 in this value!!!
};

STATIC_ASSERT(!FLAG_ISSET(DEAD_PTE, PTE_V), "DEAD_PTE cannot be a valid PTE!");

static bool g_initialized = false;
static pthread_once_t g_auto_dedicate_once = PTHREAD_ONCE_INIT;

int dangless_dedicate_vmem(void *start, void *end) {
  g_initialized = true;

  DPRINTF("dedicating virtual memory: " FMT_PTR " - " FMT_PTR "\n", start, end);
  return vp_free_region(start, end);
}

// TODO: make this platform-specific
static void auto_dedicate_vmem(void) {
  const size_t pte_addr_mult = 1uL << pt_level_shift(PT_L4);
  pte_t *ptroot = pt_root();

  size_t nentries_dedicated = 0;

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    if (ptroot[i])
      continue;

    int result = dangless_dedicate_vmem(
      (void *)(i * pte_addr_mult),
      (void *)((i + 1) * pte_addr_mult)
    );

    if (result == 0) {
      nentries_dedicated++;

      if (nentries_dedicated >= DANGLESS_AUTO_DEDICATE_MAX_PML4ES) {
        DPRINTF("reached auto-dedication limit of %zu PML4 entries\n", DANGLESS_AUTO_DEDICATE_MAX_PML4ES);
        break;
      }
    }
  }

  DPRINTF("auto-dedicated %zu PML4 entries!\n", nentries_dedicated);
}

static __thread unsigned g_hook_depth = 0;

bool dangless_hook_running(void) {
  return g_hook_depth > 0;
}

static bool do_hook_enter(const char *func_name) {
  //DPRINTF_NOMALLOC("entering hook from %s (depth = %d)...\n", func_name, g_hook_depth);

  if (g_hook_depth > 0 || UNLIKELY(!in_kernel_mode()))
    return false;

  g_hook_depth++;

  //DPRINTF_NOMALLOC("entered hook!\n");
  return true;
}

static void do_hook_exit(const char *func_name) {
  ASSERT(g_hook_depth > 0, "Unbalanced HOOK_ENTER/HOOK_EXIT?!");

  g_hook_depth--;

  //DPRINTF("exited hook from %s (new depth: %d)\n", func_name, g_hook_depth);
}

#define HOOK_ENTER() do_hook_enter(__func__)
#define HOOK_EXIT() do_hook_exit(__func__)

#define HOOK_RETURN(V) \
  do { \
    __typeof((V)) __hook_result = (V); \
    HOOK_EXIT(); \
    return __hook_result; \
  } while (0)

static void *do_vremap(void *p, size_t sz, const char *func_name) {
  // "If size is zero, the behavior is implementation defined (null pointer may be returned, or some non-null pointer may be returned that may not be used to access storage, but has to be passed to free)." (http://en.cppreference.com/w/c/memory/malloc)
  if (!p || !sz)
    return NULL;

  void *remapped_ptr;
  int result = vremap_map(p, sz, OUT &remapped_ptr);

  if (UNLIKELY(result == EVREM_NO_VM && !g_initialized)) {
    DPRINTF("auto-dedicating available virtual memory to dangless...\n");

    pthread_once(&g_auto_dedicate_once, auto_dedicate_vmem);

    DPRINTF("re-trying vremap...\n");
    return do_vremap(p, sz, func_name);
  } else if (result < 0) {
    DPRINTF("failed to remap %s's %p of size %zu; falling back to proxying (result %d)\n", func_name, p, sz, result);
    return p;
  }

  return remapped_ptr;
}

void *dangless_malloc(size_t size) {
  void *p = sysmalloc(size);
  if (!HOOK_ENTER())
    return p;

  HOOK_RETURN(do_vremap(p, size, "malloc"));
}

void *dangless_calloc(size_t num, size_t size) {
  void *p = syscalloc(num, size);
  if (!HOOK_ENTER())
    return p;

  // TODO: "Due to the alignment requirements, the number of allocated bytes is not necessarily equal to num*size." (http://en.cppreference.com/w/c/memory/calloc)
  HOOK_RETURN(do_vremap(p, num * size, "calloc"));
}

int dangless_posix_memalign(void **pp, size_t align, size_t size) {
  int result = sysmemalign(pp, align, size);
  if (result != 0 || !HOOK_ENTER())
    return result;

  *pp = do_vremap(*pp, size, "posix_memalign");
  HOOK_RETURN(0);
}

// Invalidates a virtual page that was remapped with virtual_remap(), by marking its PTE as dead.
// This allows us to detect a dangling pointer attempted access if we hook into the pagefault interrupt.
static void virtual_invalidate_pte(pte_t *ppte) {
  *ppte = DEAD_PTE;
}

static void virtual_invalidate(void *p, size_t npages) {
  DPRINTF("invalidating %zu virtual pages starting at %p...\n", npages, p);

  size_t page = 0;
  while (page < npages) {
    pte_t *ppte;
    enum pt_level level = pt_walk(PG_OFFSET(p, page), PGWALK_FULL, OUT &ppte);
    ASSERT0(level == PT_L1);
    ASSERT0(FLAG_ISSET(*ppte, PTE_V));

    // optimised for invalidating adjecent PTEs in a PT
    size_t pte_base_offset = pt_level_offset((vaddr_t)p, PT_L1);
    size_t nptes = MIN(PT_NUM_ENTRIES - pte_base_offset, npages - page);

    size_t pte_offset;
    for (pte_offset = 0; pte_offset < nptes; pte_offset++) {
      virtual_invalidate_pte(&ppte[pte_offset]);
      tlb_flush_one(PG_OFFSET(p, page + pte_offset));
    }

    page += nptes;
  }
}

void dangless_free(void *p) {
  // compatibility with the libc free()
  if (!p)
    return;

  if (!HOOK_ENTER()) {
    sysfree(p);
    return;
  }

  void *original_ptr = p;
  int result = vremap_resolve(p, OUT &original_ptr);

  if (result == 0) {
    size_t npages = sysmalloc_usable_pages(original_ptr);
    virtual_invalidate(p, npages);
  } else if (result < 0) {
    DPRINTF("failed to determine whether %p was remapped: assume not (result %d)\n", p, result);
  } else {
    DPRINTF("vremap_resolve returned %d, assume no remapping\n", result);
  }

  sysfree(original_ptr);

  HOOK_EXIT();
}

void *dangless_realloc(void *p, size_t new_size) {
  if (!HOOK_ENTER())
    return sysrealloc(p, new_size);

  void *original_ptr = p;
  bool was_remapped = false;

  if (p) {
    int result = vremap_resolve(p, OUT &original_ptr);

    if (result == 0) {
      was_remapped = true;
    } else if (result < 0) {
      DPRINTF("failed to determine whether %p was remapped: assume not (result %d)\n", p, result);
    } else {
      DPRINTF("vremap_resolve returned %d, assume no remapping\n", result);
    }
  }

  if (!was_remapped) {
    // either realloc() was called with NULL, or the original pointer was not remaped, so we get to treat this as a new allocation
    HOOK_RETURN(do_vremap(sysrealloc(p, new_size), new_size, "realloc"));
  }

  // TODO: this is imprecise, there's also the malloc header to consider!
  const size_t new_npages = ROUND_UP(new_size, PGSIZE) / PGSIZE;
  const size_t old_npages = sysmalloc_usable_pages(p);
  DPRINTF("new_npages = %zu, old_npages = %zu\n", new_npages, old_npages);

  void *newp = sysrealloc(original_ptr, new_size);
  if (!newp)
    HOOK_RETURN(NULL);

  if (newp == original_ptr) {
    // we can potentially do it in-place

#if DGLMALLOC_DEBUG
    // verify that the physical page also didn't change
    paddr_t old_pa_page = pt_resolve_page(original_ptr, NULL);
    paddr_t new_pa_page = pt_resolve_page(newp, NULL);
    ASSERT(old_pa_page == new_pa_page, "sysrealloc() performed virtual reallocation in-place, but not the physical reallocation! original_ptr = newp = %p, but old_pa = %p != new_pa = %p", newp, (void *)old_pa_page, (void *)new_pa_page);
#endif

    if (new_npages == old_npages) {
      // no change in terms of location or in terms of size: nothing to do, just return the original pointer
      HOOK_RETURN(p);
    } else if (new_npages < old_npages) {
      // shrink in-place: only invalidate the pages at the end
      virtual_invalidate(PG_OFFSET(p, new_npages), old_npages - new_npages);
      HOOK_RETURN(p);
    }
  }

  // there's no in-place reallocation possible: either the location changed, or new pages were requested: invalidate the whole original region, and create a new remapped region
  // NOTE that it's possible that we'll still end up with result == p, if the virtual memory region after p is free and can be used to accommodate new_size - old_size
  virtual_invalidate(p, old_npages);
  HOOK_RETURN(do_vremap(newp, new_size, "realloc"));
}

// strong overrides of the glibc memory allocation symbols
// --whole-archive has to be used when linking against the library so that these symbols will get picked up instead of the glibc ones
#if DANGLESS_OVERRIDE_SYMBOLS
  #define STRONG_ALIAS(ALIASNAME, OVERRIDENAME) \
    extern __typeof(OVERRIDENAME) ALIASNAME __attribute__((alias(#OVERRIDENAME)))

  STRONG_ALIAS(malloc, dangless_malloc);
  STRONG_ALIAS(calloc, dangless_calloc);
  STRONG_ALIAS(realloc, dangless_realloc);
  STRONG_ALIAS(posix_memalign, dangless_posix_memalign);
  STRONG_ALIAS(free, dangless_free);
#endif
