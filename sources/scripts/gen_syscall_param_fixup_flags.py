#!/usr/bin/env python3
import sys
import os.path

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(CURRENT_DIR, "..", "..", "vendor", "linux-syscallmd"))
import linux_syscallmd

PARAM_FLAG_NAME_PREFIX = "SYSCALL_PARAM_"

PARAM_FLAG_CHECKERS = {
  "USER_PTR": lambda p: p.is_user_pointer,
  "IOVEC": lambda p: "struct iovec" in p.type,
  "PTR_PTR": lambda p: p.is_user_pointer and p.type.index("__user *") != p.type.rindex("__user *"),
  "MSGHDR": lambda p: "struct msghdr" in p.type
}

def emit_syscall_param_fixup_flags(outs, syscalls):
  outs.write("""#ifndef __NR_write
  #error __NR_write missing: you have to include <sys/syscall.h> before you can include this file
#endif
""")

  for syscall in syscalls:
    params_flags = []

    for param in syscall.params:
      pflags = []

      for flag_name, flag_checker in PARAM_FLAG_CHECKERS.items():
        if flag_checker(param):
          pflags.append(flag_name)

      params_flags.append(pflags)

    # mark the last interesting parameter with the "LAST" flag, or the first one if none of them were interesting
    for i in reversed(range(len(params_flags))):
      if len(params_flags[i]) > 0 or i == 0:
        params_flags[i].append("LAST")
        break

    outs.write("""
  #ifdef {0}
    [{0}] = {{
""".format(syscall.number_macro))

    for i, pflags in enumerate(params_flags):
      outs.write("\t\t// {} {}\n".format(syscall.params[i].type, syscall.params[i].name))
      outs.write("\t\t(uint8_t)SYSCALL_PARAM_VALID")

      for pflag in pflags:
        outs.write(" | (uint8_t)")
        outs.write(PARAM_FLAG_NAME_PREFIX)
        outs.write(pflag)

      if i < len(params_flags) - 1:
        outs.write(",\n")

    outs.write("""
    }}, // end {}
  #endif
""".format(syscall.name))


if __name__ == "__main__":
  linux_headers_dir = sys.argv[1]
  syscalls = linux_syscallmd.load_from_headers(linux_headers_dir)
  emit_syscall_param_fixup_flags(sys.stdout, syscalls)
