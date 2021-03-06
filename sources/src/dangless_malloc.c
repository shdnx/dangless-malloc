#define _GNU_SOURCE

#include "dangless/config.h"

#if DANGLESS_CONFIG_TRACE_HOOKS_BACKTRACE
  #include <execinfo.h>
#endif

#if DANGLESS_CONFIG_SUPPORT_MULTITHREADING
  #include <pthread.h>
#else
  #include "dangless/pthread_mock.h"
#endif

#include <stdlib.h> // abort

#include "dangless/common.h"
#include "dangless/common/statistics.h"
#include "dangless/common/printf_nomalloc.h"
#include "dangless/dangless_malloc.h"
#include "dangless/virtmem.h"
#include "dangless/virtmem_alloc.h"

#include "dangless/platform/sysmalloc.h"
#include "dangless/platform/virtual_remap.h"

#if DANGLESS_CONFIG_DEBUG_DGLMALLOC
  #define LOG(...) vdprintf(__VA_ARGS__)
  #define LOG_NOMALLOC(...) vdprintf_nomalloc(__VA_ARGS__)
#else
  #define LOG(...) /* empty */
  #define LOG_NOMALLOC(...) /* empty */
#endif

static pthread_once_t g_auto_dedicate_once = PTHREAD_ONCE_INIT;
static bool g_auto_dedicate_happened = false;

STATISTIC_DEFINE_COUNTER(st_num_allocations);
STATISTIC_DEFINE_COUNTER(st_num_allocations_2mb_plus);
STATISTIC_DEFINE_COUNTER(st_num_allocations_1gb_plus);

STATISTIC_DEFINE_COUNTER(st_num_reallocations);
STATISTIC_DEFINE_COUNTER(st_num_reallocated_vpages);

STATISTIC_DEFINE_COUNTER(st_num_deallocations);
STATISTIC_DEFINE_COUNTER(st_num_deallocated_vpages);

int dangless_dedicate_vmem(void *start, void *end) {
  LOG("dedicating virtual memory: %p - %p\n", start, end);
  return vp_free_region(start, end);
}

// Look for any unused PML4 entries that each represent 512 GB of virtual memory, and dedicate them to the Dangless virtual memory allocator.
// TODO: make this platform-specific
static void auto_dedicate_vmem(void) {
  // TODO: maybe refactor this? There's an almost identical function in tests/dune/basics.c...
  const size_t pte_mapped_size = 1uL << pt_level_shift(PT_L4);
  pte_t *ptroot = pt_root();

  size_t nentries_dedicated = 0;

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    if (ptroot[i])
      continue;

    int result = dangless_dedicate_vmem(
      (void *)(i * pte_mapped_size),
      (void *)((i + 1) * pte_mapped_size)
    );

    if (result == 0)
      nentries_dedicated++;
  }

  LOG("auto-dedicated %zu PML4 entries!\n", nentries_dedicated);
  g_auto_dedicate_happened = true;
}

static THREAD_LOCAL unsigned g_hook_depth = 0;
static THREAD_LOCAL const char *g_hook_funcname = NULL;

bool dangless_is_hook_running(void) {
  return g_hook_depth > 0;
}

static bool do_hook_enter(const char *func_name, void *retaddr) {
  //LOG_NOMALLOC("entering hook %s...\n", func_name);

  if (UNLIKELY(dangless_is_hook_running())) {
    LOG_NOMALLOC("failed to enter %s: already running %s\n", func_name, g_hook_funcname);
    return false;
  }

  if (UNLIKELY(!in_kernel_mode())) {
    //LOG_NOMALLOC("failed to enter hook %s: not in kernel mode yet\n", func_name);
    return false;
  }

  g_hook_depth++;
  g_hook_funcname = func_name;

#if DANGLESS_CONFIG_TRACE_HOOKS
  #if DANGLESS_CONFIG_TRACE_HOOKS_BACKTRACE
    vdprintf("\nentering %s\n", func_name);
    dprintf_scope_enter(func_name);

    void *backtrace_buffer[DANGLESS_CONFIG_TRACE_HOOKS_BACKTRACE_BUFSIZE];
    const int backtrace_len = backtrace(REF backtrace_buffer, ARRAY_LENGTH(backtrace_buffer));

    dprintf("-- backtrace (%d frames) --\n", backtrace_len);
    // we don't want to use backtrace_symbols(), as it involves a call to malloc()
    backtrace_symbols_fd(backtrace_buffer, backtrace_len, /*fd=*/2 /*stderr*/);
    dprintf("-- end backtrace --\n\n");
  #else
    dprintf("\nentering %s (called from %p)\n", func_name, retaddr);
    dprintf_scope_enter(func_name);
  #endif
#endif

  return true;
}

static void do_hook_exit(const char *func_name, void *retaddr) {
  ASSERT(dangless_is_hook_running(), "Unbalanced HOOK_ENTER/HOOK_EXIT?!");

  g_hook_depth--;
  g_hook_funcname = NULL;

#if DANGLESS_CONFIG_TRACE_HOOKS
  dprintf_scope_exit(func_name);
  vdprintf("exited %s (returning to %p)\n", func_name, retaddr);
#endif
}

// GCC extension
#define RETURN_ADDR() __builtin_extract_return_addr(__builtin_return_address(0))

#define HOOK_ENTER() do_hook_enter(__func__, RETURN_ADDR())
#define HOOK_EXIT() do_hook_exit(__func__, RETURN_ADDR())

#define HOOK_RETURN(V) \
  do { \
    typeof((V)) __hook_result = (V); \
    HOOK_EXIT(); \
    return __hook_result; \
  } while (0)

static void *do_vremap(void *p, size_t sz, const char *func_name) {
  // "If size is zero, the behavior is implementation defined (null pointer may be returned, or some non-null pointer may be returned that may not be used to access storage, but has to be passed to free)." (http://en.cppreference.com/w/c/memory/malloc)
  if (!p || !sz)
    return NULL;

  // we need to grab the actual allocated size, because it can be higher than the requested size, and the difference can sometimes fall across a page boundary, making it important
  const size_t actual_sz = sysmalloc_usable_size(p);

  STATISTIC_UPDATE() {
    if (actual_sz >= 1024 * 1024 * 1024) {
      st_num_allocations_1gb_plus++;
    } else if (actual_sz >= 2 * 1024 * 1024) {
      st_num_allocations_2mb_plus++;
    }
  }

  int result;
  void *remapped_ptr;

retry:
  result = vremap_map(p, actual_sz, OUT &remapped_ptr);

#if DANGLESS_CONFIG_AUTO_DEDICATE_PML4ES
  if (UNLIKELY(result == EVREM_NO_VM && !g_auto_dedicate_happened)) {
    LOG("auto-dedicating available virtual memory to Dangless...\n");

    pthread_once(&g_auto_dedicate_once, auto_dedicate_vmem);

    LOG("re-trying vremap...\n");
    goto retry;
  }
#endif

  if (result < 0) {
    #if DANGLESS_CONFIG_ALLOW_SYSMALLOC_FALLBACK
      // TODO: this should set a flag or something, to ensure that the whole thing is not re-tried unless we get more virtual memory
      LOG("failed to remap %s's %p of size %zu: %s (code %d); falling back to proxying sysmalloc\n", func_name, p, actual_sz, vremap_diag(result), result);
      return p;
    #else
      fprintf_nomalloc(stderr, "[dangless_malloc] FATAL ERROR: failed to remap %s's %p of size %zu: %s (code %d); falling back to sysmalloc proxying is disallowed, failing\n", func_name, p, actual_sz, vremap_diag(result), result);
      abort();
    #endif
  }

  return remapped_ptr;
}

void *dangless_malloc(size_t size) {
  void *p = sysmalloc(size);
  if (!HOOK_ENTER())
    return p;

  STATISTIC_UPDATE() {
    st_num_allocations++;
  }

  //LOG("sysmalloc p = %p, size = %zu, endp = %p\n", p, size, (char *)p + size);

  HOOK_RETURN(do_vremap(p, size, "malloc"));
}

void *dangless_calloc(size_t num, size_t size) {
  void *p = syscalloc(num, size);
  if (!HOOK_ENTER())
    return p;

  STATISTIC_UPDATE() {
    st_num_allocations++;
  }

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
  *ppte = PTE_INVALIDATED;
}

static void virtual_invalidate(void *p, size_t npages) {
  p = (void *)ROUND_DOWN((vaddr_t)p, PGSIZE);

  LOG("invalidating %zu virtual pages starting at %p...\n", npages, p);

  size_t page_offset = 0;
  while (page_offset < npages) {
    void *ppage = PG_OFFSET(p, page_offset);

    pte_t *ppte;
    enum pt_level level = pt_walk(ppage, PGWALK_FULL, OUT &ppte);
    ASSERT0(level == PT_L1);
    ASSERT0(FLAG_ISSET(*ppte, PTE_V));

    // optimised for invalidating adjecent PTEs in a PT
    size_t pte_base_offset = pt_level_offset((vaddr_t)ppage, PT_L1);
    size_t nptes = MIN(PT_NUM_ENTRIES - pte_base_offset, npages - page_offset);

    size_t pte_offset;
    for (pte_offset = 0; pte_offset < nptes; pte_offset++) {
      virtual_invalidate_pte(&ppte[pte_offset]);
      tlb_flush_one(PG_OFFSET(ppage, pte_offset));
    }

    page_offset += nptes;
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

  STATISTIC_UPDATE() {
    st_num_deallocations++;
  }

  void *original_ptr = p;
  int result = vremap_resolve(p, OUT &original_ptr);
  LOG("vremap_resolve(%p) => %s (%d); %p\n", p, vremap_diag(result), result, original_ptr);

  if (result == VREM_OK) {
    const size_t npages = sysmalloc_spanned_pages(original_ptr);
    virtual_invalidate(p, npages);

    STATISTIC_UPDATE() {
      st_num_deallocated_vpages += npages;
    }
  } /*else if (result < 0) {
    LOG("failed to determine whether %p was remapped: assume not (result %d)\n", p, result);
  } else {
    LOG("vremap_resolve returned %d, assume no remapping\n", result);
  }*/

  sysfree(original_ptr);

  HOOK_EXIT();
}

void *dangless_realloc(void *p, size_t new_size) {
  LOG("entering realloc...\n");

  if (!HOOK_ENTER())
    return sysrealloc(p, new_size);

  STATISTIC_UPDATE() {
    if (p) {
      st_num_reallocations++;
    } else {
      st_num_allocations++;
    }
  }

  LOG("realloc(p = %p, new_size = %zu)\n", p, new_size);

  void *original_ptr = p;
  bool was_remapped = false;

  if (p) {
    // TODO: there's some bullshit going on here, LOG() is screwing up "result"???
    const int result = vremap_resolve(p, OUT &original_ptr);
    //LOG("vremap_resolve(%p) => %s (%d), %p\n", p, vremap_diag(result), result, original_ptr);
    //fprintf(stderr, "2 vremap_resolve(%p) => %s (%d), %p\n", p, vremap_diag(result), result, original_ptr);

    if (result == 0) {
      was_remapped = true;
    } else if (result < 0) {
      //LOG("failed to determine whether %p was remapped: assume not (result %d)\n", p, result);
    } /*else {
      LOG("vremap_resolve returned %d, assume no remapping\n", result);
    }*/
  }

  if (!was_remapped) {
    // either realloc() was called with NULL, or the original pointer was not remaped, so we get to treat this as a new allocation
    void *const newp = do_vremap(sysrealloc(p, new_size), new_size, "realloc");
    LOG("pointer is NULL or not remapped; treating it as new allocation => %p\n", newp);
    HOOK_RETURN(newp);
  }

  const size_t actual_old_size = sysmalloc_usable_size(original_ptr);

  //const size_t old_npages = sysmalloc_usable_pages(original_ptr);
  const size_t old_npages = NUM_SPANNED_PAGES(original_ptr, actual_old_size);

  STATISTIC_UPDATE() {
    st_num_reallocated_vpages += old_npages;
  }

  void *newp = sysrealloc(original_ptr, new_size);
  if (!newp)
    HOOK_RETURN(NULL);

  // unfortunately, realloc() is allowed to give more space than requested for; e.g. a realloc() asking for 4080 bytes, it might get 4088 bytes - and those extra bytes can be past a page boundary, meaning we have to account for them...
  const size_t actual_new_size = sysmalloc_usable_size(newp);

  LOG("original_ptr = %p (usable size: %zu), newp = %p (usable size: %zu)\n", original_ptr, actual_old_size, newp, actual_new_size);

  ASSERT0(actual_new_size >= new_size);

  //ASSERT(new_size == actual_new_size, "Wtf are you doing, sysrealloc()? new_size = %zu, malloc_usable_size => %zu\n", new_size, actual_new_size);

  if (newp == original_ptr) {
    // we can potentially do it in-place
    // TODO: we could do it in-place even if the system allocator couldn't; we just have to update the mapping of all of the old pages

    // TODO: does this logic even make sense? we should take a look at the ending virtual page number of the original allocation and the ending virtual page number of the reallocation instead of calculating the number of pages - this would make the logic a lot simpler also

    //const size_t new_npages = ROUND_UP(new_size, PGSIZE) / PGSIZE;
    const size_t new_npages = NUM_SPANNED_PAGES(newp, actual_new_size);

    LOG("new_npages = %zu, old_npages = %zu\n", new_npages, old_npages);

#if DANGLESS_CONFIG_DEBUG_DGLMALLOC
    // verify that the physical page also didn't change
    paddr_t old_pa_page = pt_resolve_page(original_ptr, OUT_IGNORE);
    paddr_t new_pa_page = pt_resolve_page(newp, OUT_IGNORE);
    ASSERT(old_pa_page == new_pa_page, "sysrealloc() performed virtual reallocation in-place, but not the physical reallocation! original_ptr = newp = %p, but old_pa = %p != new_pa = %p", newp, (void *)old_pa_page, (void *)new_pa_page);
#endif

    if (new_npages == old_npages) {
      // no change in terms of location or in terms of size: nothing to do, just return the original pointer
      LOG("in-place realloc, no changes in pages => %p\n", p);
      HOOK_RETURN(p);
    } else if (new_npages < old_npages) {
      // shrink in-place: only invalidate the pages at the end
      //void *const p_invalidate_from = PG_OFFSET(p, new_npages);
      void *const p_invalidate_from = (void *)ROUND_UP((vaddr_t)p + actual_new_size, PGSIZE);
      const size_t num_invalid_pages = old_npages - new_npages;

      virtual_invalidate(p_invalidate_from, num_invalid_pages);
      LOG("shrinking in-place realloc => %p\n", p);
      HOOK_RETURN(p);
    }
    // otherwise we're growing in-place that we cannot guarantee to be able to do in virtual memory, so just create a new region
  }

  // there's no safe in-place reallocation possible: either the location changed, or new pages were requested: invalidate the whole original region, and create a new remapped region
  // NOTE that it's possible that we'll still end up with result == p, if the virtual memory region after p is free and can be used to accommodate new_size - old_size
  virtual_invalidate(p, old_npages);

  void *const newvp = do_vremap(newp, new_size, "realloc");
  LOG("=> %p\n", newvp);
  HOOK_RETURN(newvp);
}

void *dangless_get_canonical(void *p) {
  void *original_ptr;
  int result = vremap_resolve(p, OUT &original_ptr);
  if (result < 0) {
    LOG("vremap_resolve failed: %s (%d)\n", vremap_diag(result), result);
    return NULL;
  }

  return original_ptr;
}

/*size_t dangless_usable_size(void *p) {
  void *c = dangless_get_canonical(p);
  if (!c)
    return 0;

  return malloc_usable_size(c);
}*/

// strong overrides of the glibc memory allocation symbols
// --whole-archive has to be used when linking against the library so that these symbols will get picked up instead of the glibc ones
#if DANGLESS_CONFIG_OVERRIDE_SYMBOLS
  #define STRONG_ALIAS(ALIASNAME, OVERRIDENAME) \
    extern typeof(OVERRIDENAME) ALIASNAME __attribute__((alias(#OVERRIDENAME)))

  STRONG_ALIAS(malloc, dangless_malloc);
  STRONG_ALIAS(calloc, dangless_calloc);
  STRONG_ALIAS(realloc, dangless_realloc);
  STRONG_ALIAS(posix_memalign, dangless_posix_memalign);
  STRONG_ALIAS(free, dangless_free);
#endif
