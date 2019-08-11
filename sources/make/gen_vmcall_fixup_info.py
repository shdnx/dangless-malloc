#!/usr/bin/env python3
import sys
import os.path
from typing import List, TextIO

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(CURRENT_DIR, "..", "..", "vendor", "linux-syscallmd"))
import linux_syscallmd


def determine_param_fixup_type(param: linux_syscallmd.SystemCallParameter) -> str:
  if "struct iovec" in param.type:
    return "IOVEC"
  elif param.is_user_pointer and param.type.index("__user *") != param.type.rindex("__user *"):
    return "PTR_PTR"
  elif "struct msghdr" in param.type:
    return "MSGHDR"
  elif param.is_user_pointer:
    return "FLAT_PTR"
  else:
    return "NONE"


def emit_vmcall_fixup_infos(
  outs: TextIO,
  syscalls: List[linux_syscallmd.SystemCall]
) -> None:
  outs.write("""#ifndef __NR_write
  #error __NR_write missing: you have to include <sys/syscall.h> before you can include this file
#endif
""")

  for syscall in syscalls:
    outs.write(f"""
  #ifdef {syscall.number_macro}
    [{syscall.number_macro}] = {{
      .params = {{
""")

    for param_index, param in enumerate(syscall.params):
      outs.write(f"""
        [{param_index}] = {{
          .fixup_type = VMCALL_PARAM_{determine_param_fixup_type(param)}
        }},
""")

    outs.write(f"""
      }} // end .params
    }}, // end {syscall.name}
  #endif
""")


if __name__ == "__main__":
  linux_headers_dir = sys.argv[1]
  syscalls = linux_syscallmd.load_from_headers(linux_headers_dir)
  emit_vmcall_fixup_infos(sys.stdout, syscalls)
