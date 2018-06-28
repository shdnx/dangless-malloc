#!/usr/bin/env python3
import sys
import os.path
from functools import reduce

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(CURRENT_DIR, "..", "..", "vendor", "linux-syscallmd"))
import linux_syscallmd

def emit_userptr_info(outs, syscalls):
  outs.write("""#ifndef __NR_write
  #error __NR_write missing: you have to include <sys/syscall.h> before you can use this
#endif

#ifndef SYSCALL_USERPTR_PARAMS
  #define SYSCALL_USERPTR_PARAMS(NAME, ...) /* empty */
#endif
""")

  for syscall in syscalls:
    # skip syscall if it doesn't have any user pointer parameters
    if not reduce(lambda acc, param: acc or param.is_user_pointer, syscall.params, False):
      continue

    outs.write("\n#ifdef {}\n".format(syscall.number_macro))

    outs.write("\tSYSCALL_USERPTR_PARAMS(")
    outs.write(syscall.number_macro)
    outs.write(", ")
    outs.write(syscall.name)

    for i, param in enumerate(syscall.params):
      if param.is_user_pointer:
        outs.write(", ")
        outs.write(str(i + 1))

    outs.write(")\n")

    outs.write("#endif\n")

  outs.write("""
#undef SYSCALL_USERPTR_PARAMS
""")


if __name__ == "__main__":
  linux_headers_dir = sys.argv[1]
  syscalls = linux_syscallmd.load_from_headers(linux_headers_dir)
  emit_userptr_info(sys.stdout, syscalls)
