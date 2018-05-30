import sys, os, re, io, functools

# NOTE: a problem with this script is that it doesn't, and cannot take into account system call declarations that are inside #if and #ifdefs, such as sys_clone(). Currently this (in kernel version 4.4.0-67-generic) this doesn't seem to pose a problem (besides issuing some -Woverride-init warnings), but it's something to be considered.

LINUX_HEADERS_DIR = sys.argv[1]

class SystemCallParameter:
  def __init__(self, name, ty):
    self.name = name
    self.type = ty


  def isAnonymous(self):
    return self.name is None


  def isUserPointer(self):
    return "__user *" in self.type


class SystemCall:
  def __init__(self, name, returnType):
    self.name = name.strip()
    self.returnType = returnType.strip()
    self.params = []


# asmlinkage long sys_io_getevents(aio_context_t ctx_id,
#         long min_nr,
#         long nr,
#         struct io_event __user *events,
#         struct timespec __user *timeout);

sysfuncStartPattern = re.compile('^asmlinkage (.*?) sys[_](.*?)[(]')

def separateParameters(text):
  paramBuffer = io.StringIO()

  for c in text:
    if c == ")":
      # the function declaration is over, we're done
      yield paramBuffer.getvalue().strip()
      yield True # indicate we're completely done
      return
    elif c == ",":
      # the parameter is done
      yield paramBuffer.getvalue().strip()

      # reset the buffer before continuing
      paramBuffer.truncate(0)
      paramBuffer.seek(0)
    else:
      paramBuffer.write(c)

  # indicate that we haven't encountered a ), parsing should continue in the next line
  yield False


def parseParameter(paramText):
  paramText = paramText.strip()
  paramType = None
  paramName = None

  # this wouldn't work for complicated things like function types or array types, but it's fine for the simpler cases, and system calls will never take complicated things as parameters
  lastSpaceIndex = paramText.rfind(" ")
  if lastSpaceIndex == -1:
    paramType = paramText
  else:
    paramType = paramText[: lastSpaceIndex].strip()
    paramName = paramText[lastSpaceIndex + 1 :].strip()

    if paramName[0] == "*":
      paramType += " *"
      paramName = paramName[1:]

    if paramName == "":
      paramName = None

  if paramName is not None and re.match('^[a-zA-Z_][a-zA-Z_0-9]*$', paramName) is None:
    raise NotImplementedError("Unable to parse parameter '{}': parsed name '{}' seems invalid!".format(paramText, paramName))

  return SystemCallParameter(paramName, paramType)


SYSCALLS = []

with open(os.path.join(LINUX_HEADERS_DIR, "include/linux/syscalls.h"), "r") as fp:
  currentSysCall = None

  for line in fp.readlines():
    paramsText = None

    if currentSysCall is None:
      m = sysfuncStartPattern.match(line)
      if m is None:
        continue

      returnType = m.group(1)
      name = m.group(2)

      #if name not in KNOWN_SYSCALLS:
      #  sys.stderr.write("Omitting {}: no syscall number assigned?\n".format(name))
      #  continue

      currentSysCall = SystemCall(name, returnType)

      paramsText = line[len(m.group(0)) :]
    else:
      paramsText = line

    # parse parameters
    for paramText in separateParameters(paramsText):
      if paramText is False:
        # we're finished with parsing the line, but not finished with parsing the syscall declaration
        break
      elif paramText is True:
        # we're done with parsing this syscall declaration
        SYSCALLS.append(currentSysCall)
        currentSysCall = None
        break
      else:
        currentSysCall.params.append(parseParameter(paramText))

os = sys.stdout
os.write("""// This file needs <sys/syscall.h> to be included, for access to the __NR_* syscall number macros

#ifndef SYSCALL_SIGNATURE
  #define SYSCALL_SIGNATURE(RETTYPE, NAME, NUM_PARAMS, ...) /* empty */
#endif

#ifndef SYSCALL_USERPTR_PARAMS
  #define SYSCALL_USERPTR_PARAMS(NAME, ...) /* empty */
#endif
""")

for syscall in SYSCALLS:
  os.write("\n#ifdef __NR_{f.name}\n".format(f = syscall))

  # write SYSCALL_SIGNATURE()
  os.write("\tSYSCALL_SIGNATURE({}, {}, {}".format(syscall.returnType, syscall.name, len(syscall.params)))

  for param in syscall.params:
    os.write(", {} /* {} */".format( \
      param.type, \
      param.name if param.name is not None else "unnamed" \
    ))

  os.write(")\n")

  # write SYSCALL_USERPTR_PARAMS(), but only if there are actually any user pointer parameters
  if functools.reduce(lambda acc, param: acc or param.isUserPointer(), syscall.params):
    os.write("\tSYSCALL_USERPTR_PARAMS({f.name}".format(f = syscall))

    for i, param in enumerate(syscall.params):
      if param.isUserPointer():
        os.write(", {}".format(i + 1))

    os.write(")\n")

  os.write("#endif\n")

os.write("""
#undef SYSCALL_SIGNATURE
#undef SYSCALL_USERPTR_PARAMS
""")
