#include "vmcall_hooks.h"
#include "vmcall_handle_fork.h"

#include "dune.h"

static ptent_t *g_host_pgroot;

void vmcall_handle_fork(REF u64 *pretaddr) {
  g_host_pgroot = pgroot;

  // hijack vmcall return address
  REF *pretaddr = (u64)&vmcall_fork_landing;
}

extern void vmcall_fork_landing(void);

void vmcall_fork_landing_child(void) {
  // TODO: copy entries from g_host_ptroot to ptroot
  vdprintf("Child process! pgroot = %p, host pgroot = %p\n", pgroot, g_host_pgroot);
}
