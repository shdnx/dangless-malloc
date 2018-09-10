.text

.globl vmcall_fork_landing
vmcall_fork_landing:
  test %rax, %rax
  jnz end_hijack

  // this is the child process, call the landing hook
  // before we can do that, we have to save the registers rax, rcx, rdx, rdi, rsi, r8-11 before we can call into a C function, as they have to be restored for the syscall caller
  // TODO: this is identical to VMCALL_HOOK_SAVE_REGS/VMCALL_HOOK_RESTORE_REGS from dune.S currently; maybe refactor?
  pushq %rcx
  pushq %rax
  pushq %r9
  pushq %r8
  pushq %r10
  pushq %rdx
  pushq %rsi
  pushq %rdi

  call vmcall_fork_landing_child

  popq %rdi
  popq %rsi
  popq %rdx
  popq %r10
  popq %r8
  popq %r9
  popq %rax
  popq %rcx

end_hijack:
  jmp *g_current_syscall_return_addr
