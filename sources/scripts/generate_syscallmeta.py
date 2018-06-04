import sys, os, re, io, functools

# NOTE: a problem with this script is that it doesn't, and cannot take into account system call declarations that are inside #if and #ifdefs, such as sys_clone(). Currently this (in kernel version 4.4.0-67-generic) this doesn't seem to pose a problem (besides issuing some -Woverride-init warnings), but it's something to be considered.

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


  def getNumberMacro(self):
    return "__NR_{}".format(self.name)


def parseSystemCalls(filePath):
  # System calls are defined similarly to this:
  #
  #   asmlinkage long sys_io_getevents(aio_context_t ctx_id,
  #           long min_nr,
  #           long nr,
  #           struct io_event __user *events,
  #           struct timespec __user *timeout);

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


  def isParameterType(text):
    return text.endswith("_t") \
      or text in [ "int", "long", "short", "char", "const" ]


  def isIncompleteParameterType(text):
    return text in [ "struct", "enum", "const", "volatile" ]


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

    # sometimes the parameter name is missing, but the type is multiple words - try to detect these cases and fix them
    if paramName is not None:
      paramName = paramName.strip()

      if isParameterType(paramName) or isIncompleteParameterType(paramType):
        paramType += " " + paramName
        paramName = None

    # don't emit the "no parameters" pseudo-parameter, e.g. gettid(void)
    if paramType == "void" and paramName is None:
      return None

    return SystemCallParameter(paramName, paramType)


  # -- end helper functions

  syscalls = []

  with open(filePath, "r") as fp:
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
          syscalls.append(currentSysCall)
          currentSysCall = None
          break
        else:
          param = parseParameter(paramText)
          if param is not None:
            currentSysCall.params.append(param)

  return syscalls


def main(argv):
  def emitSyscallSignature(outs, macroName, syscall):
    outs.write("{}({}, {}, {}, {}".format(macroName, syscall.getNumberMacro(), syscall.returnType, syscall.name, len(syscall.params)))

    for param in syscall.params:
      outs.write(", {} /* {} */".format( \
        param.type, \
        param.name if param.name is not None else "unnamed" \
      ))

    outs.write(")")


  linuxHeadersDir = argv[1]
  syscalls = parseSystemCalls(os.path.join(linuxHeadersDir, "include/linux/syscalls.h"))
  outs = sys.stdout

  outs.write("""#ifndef __NR_write
  #error __NR_write missing: you have to include <sys/syscall.h> before you can use this
#endif

#ifndef SYSCALL_SIGNATURE
  #define SYSCALL_SIGNATURE(NUM, RETTYPE, NAME, NUM_PARAMS, ...) /* empty */
#endif

#ifndef SYSCALL_PARAM
  #define SYSCALL_PARAM(POS, TYPE, NAME, IS_USER_PTR) /* empty */
#endif

#ifndef SYSCALL_END
  #define SYSCALL_END(NUM, RETTYPE, NAME, NUM_PARAMS, ...) /* empty */
#endif

#ifndef SYSCALL_USERPTR_PARAMS
  #define SYSCALL_USERPTR_PARAMS(NAME, ...) /* empty */
#endif
""")

  for syscall in syscalls:
    outs.write("\n#ifdef {}\n".format(syscall.getNumberMacro()))

    # write SYSCALL_SIGNATURE()
    outs.write("\t")
    emitSyscallSignature(outs, "SYSCALL_SIGNATURE", syscall)
    outs.write("\n")

    for index, param in enumerate(syscall.params):
      outs.write("\tSYSCALL_PARAM({}, {}, {}, {})\n".format( \
        index + 1, \
        param.type, \
        param.name if param.name is not None else "unnamed", \
        "1" if param.isUserPointer() else "0" \
      ))

    outs.write("\t")
    emitSyscallSignature(outs, "SYSCALL_END", syscall)
    outs.write("\n")

    # write SYSCALL_USERPTR_PARAMS(), but only if there are actually any user pointer parameters
    if functools.reduce(lambda acc, param: acc or param.isUserPointer(), syscall.params, False):
      outs.write("\tSYSCALL_USERPTR_PARAMS({}, {}".format(syscall.getNumberMacro(), syscall.name))

      for i, param in enumerate(syscall.params):
        if param.isUserPointer():
          outs.write(", {}".format(i + 1))

      outs.write(")\n")

    outs.write("#endif\n")

  outs.write("""
#undef SYSCALL_SIGNATURE
#undef SYSCALL_PARAM
#undef SYSCALL_END
#undef SYSCALL_USERPTR_PARAMS
""")

  return 0


if __name__ == "__main__":
  sys.exit(main(sys.argv))
