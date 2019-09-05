cmake_minimum_required(VERSION 2.6)
project(dangless_malloc VERSION 0.9 LANGUAGES C)

set(DUNE_ROOT "" CACHE PATH "Absolute path to the Dune repository. Required.")
if (NOT ${DUNE_ROOT})
  message(FATAL_ERROR "The DUNE_ROOT option must be set to the absolute path to the Dune repository")
endif()

option(OVERRIDE_SYMBOLS ON "Whether to override the malloc(), calloc(), etc. symbols by their Dangless equivalents using strong aliasing")
option(REGISTER_PREINIT ON "Whether to automatically register a .preinit_array section entry that automatically executes dangless_init() at startup")
option(SUPPORT_MULTITHREADING OFF "Whether to enable multi-threading support. EXPERIMENTAL!")
option(ALLOW_SYSMALLOC_FALLBACK ON "Whether to allow dangless to gracefully handle running out of virtual memory and continue operating as a proxy to the underlying memory allocator.")

option(COLLECT_STATISTICS ON "Whether to collect statistics during runtime about Dangless usage. If enabled, statistics are printed after every run to stderr. These are only for local developer use and are not uploaded anywhere.")
option(REPORT_RUSAGE ON "Whether to report resource usage as measured by the kernel alongside the statistics at the end of each execution to stderr.")
option(ENABLE_PERF_EVENTS ON "Whether to enable hardware performance counters about the TLB. If enabled, these are printed alongside the statistics at the end of the each execution to stderr.")

option(AUTODEDICATE_PML4ES ON "Whether Dangless should automatically dedicate any unused PML4 pagetable entries (large unused virtual memory regions) for its virtual memory allocator. If disabled, user code will have to call dangless_dedicate_vmem().")
set(CALLOC_SPECIAL_BUFSIZE 32 CACHE INTERNAL "Buffer size for special handling of the first calloc() call originating from dlsym().")
option(SYSCALLMETA_HAS_INFO ON "Whether to embed metadata information about system calls. If enabled, then system call logging will be more useful.")
set(PRINTF_NOMALLOC_BUFFER_SIZE 4096 CACHE STRING "Global buffer size for printf_nomalloc(). Printing a string longer than this will fail.")
set(DUNE_VMCALL_FIXUP_RESTORE_INITIAL_BUFFER_SIZE 32 CACHE STRING "Initial size of the vmcall fixup restoration buffer in which the address and original value of any nested pointers referenced in system call arguments is stored.")

option(DPRINTF_ENABLED ON "Whether to enable logging.")
option(DEBUG_DGLMALLOC OFF "Debug mode for dangless_malloc.c")
option(DEBUG_VIRTMEM OFF "Debug mode for virtmem.c")
option(DEBUG_VIRTMEMALLOC OFF "Debug mode for virtmem_alloc.c")
option(DEBUG_VIRTREMAP OFF "Debug mode for virtual_remap.c")
option(DEBUG_SYSMALLOC OFF "Debug mode for sysmalloc.c")
option(DEBUG_INIT OFF "Debug mode for init.c")
option(DEBUG_PHYSMEM_ALLOC OFF "Debug mode for physmem_alloc.c")
option(DEBUG_DUNE_VMCALL_PREHOOK OFF "Debug mode for vmcall_hooks.c")
option(DEBUG_DUNE_VMCALL_FIXUP OFF "Debug mode for vmcall_fixup.c")
option(DEBUG_DUNE_VMCALL_FIXUP_RESTORE OFF "Debug mode for vmcall_fixup_restore.c")
option(DEBUG_PERF_EVENTS OFF "Debug mode for common/perf_events.c")

option(TRACE_HOOKS OFF "Whether to log information whenever a Dangless memory handling function is called")
option(TRACE_HOOKS_BACKTRACE OFF "Whether to attempt to use backtrace() and backtrace_symbols() to log information about the source of Dangless memory handling function calls")
set(TRACE_HOOKS_BACKTRACE_BUFSIZE 64 CACHE STRING "Buffer size for backtracing the source of Dangless memory handling function calls")

option(TRACE_SYSCALLS OFF "Whether to log information about each system call. This information is much more useful with SYSCALLMETA_HAS_INFO enabled.")
option(TRACE_SYSCALLS_NO_WRITE_STDOUT ON "Whether to ignore write() system calls to stdout.")
option(TRACE_SYSCALLS_NO_WRITE_STDERR ON "Whether to ignore write() system calls to stderr.")

configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/buildconfig.h.in"
  "${PROJECT_BINARY_DIR}/include/dangless/build/config.h"
)

set(DANGLESS_BIN "dangless")

if(${TRACE_HOOKS_BACKTRACE})
  # -rdynamic is required with the GNU linker to make symbol names available for backtrace_symbols() and backtrace_symbols_fd()
  # see: https://linux.die.net/man/3/backtrace_symbols
  set(USER_LDFLAGS_EXTRA "-rdynamic")
endif()

configure_file(
  "${PROJECT_SOURCE_DIR}/cmake/dangless_user.make.in"
  "${PROJECT_BINARY_DIR}/dangless_user.make"
)

add_library(${DANGLESS_BIN})

target_include_directories(${DANGLESS_BIN}
  PUBLIC
    "${PROJECT_BINARY_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include"
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
    "${DUNE_ROOT}/libdune"
)

target_compile_options(${DANGLESS_BIN}
  "-Wall"
  "-Wextra"
  "-Wno-format-zero-length"
  "-Wno-sign-compare"
  "-Wno-macro-redefined"
  "-pthread"
)

target_compile_features(${DANGLESS_BIN}
  PUBLIC
    c_std_11
)

execute_process(
  COMMAND uname -r
  OUTPUT_VARIABLE LINUX_VERSION
)

set(LINUX_HEADERS_PATH "/usr/src/linux-headers-${LINUX_VERSION}")
set(LINUX_SYSCALLMD_PATH "${PROJECT_SOURCE_DIR}/../vendor/linux-syscallmd")

find_package(Python3 COMPONENTS Interpreter)

# TODO: is there really no better way than this??
# TODO: add_custom_target() instead?
add_custom_command(
  OUTPUT "${PROJECT_BINARY_DIR}/include/dangless/build/syscallmeta.inc"
  COMMAND "${Python3_EXECUTABLE} ${LINUX_SYSCALLMD_PATH}/generate_c_header.py ${LINUX_HEADERS_PATH} > ${PROJECT_BINARY_DIR}/include/dangless/build/syscallmeta.inc"
)

add_custom_command(
  OUTPUT "${PROJECT_BINARY_DIR}/include/dangless/build/vmcall_fixup_info.inc"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/gen_vmcall_fixup_info.py ${LINUX_HEADERS_PATH} > ${PROJECT_BINARY_DIR}/include/dangless/build/vmcall_fixup_info.inc"
  DEPENDS "${PROJECT_SOURCE_DIR}/scripts/gen_vmcall_fixup_info.py"
)

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(testapps)
