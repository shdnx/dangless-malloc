#include "dangless/config.h"
#include "dangless/dump.h"
#include "dangless/virtmem.h"
#include "dangless/common/types.h"
#include "dangless/common/assert.h"
#include "dangless/common/dprintf.h"

#include "init.h" // dangless_enter_dune()
#include "vmcall_handle_fork.h"

typedef ptrdiff_t off_t; // otherwise dune.h fails to compile - wtf
#include "dune.h"

// NOTE: this whole code is currently not functional, and is not used. Only 400.perlbench needs clone() support and that only in size=test, so we're not going to worry about it for now.
// Enable the option SUPPORT_MULTITHREADING to enable this code.

static ptent_t *g_host_pgroot;

extern void vmcall_fork_landing(void);

void vmcall_handle_fork(REF u64 *pretaddr) {
  g_host_pgroot = pgroot;

  // hijack vmcall return address
  REF *pretaddr = (u64)&vmcall_fork_landing;

  dprintf("host PML4:\n");
  //dump_pt_summary(stderr, pgroot, PT_L4);
  dump_pt(stderr, pgroot, PT_L4);

  dprintf("interesting mappings:\n");
  dump_mappings(stderr, 0x8000000000, 0x8000005000);
}

void vmcall_fork_landing_child(void) {
  vdprintf("Child process! pgroot = %p, host pgroot = %p\n", pgroot, g_host_pgroot);

  // TODO: this currently crashes
  dangless_enter_dune();

  if (!in_kernel_mode())
    vdprintf("WARNING: NOT IN KERNEL MODE!\n");
  else
    vdprintf("In kernel mode\n");

  dprintf("child PML4:\n");
  //dump_pt(stderr, pgroot, PT_L4);

  dprintf("interesting mappings:\n");
  dump_mappings(stderr, 0x8000000000, 0x8000005000);

  // TODO: copy entries from g_host_ptroot to ptroot
}
