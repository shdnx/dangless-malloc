cmake_minimum_required(VERSION 3.5)
project(dangless_malloc LANGUAGES C CXX)

option(OVERRIDE_SYMBOLS "Whether to override the malloc(), calloc(), etc. symbols by their Dangless equivalents using strong aliasing" ON)
option(REGISTER_PREINIT "Whether to automatically register a .preinit_array section entry that automatically executes dangless_init() at startup" ON)
option(ALLOW_SYSMALLOC_FALLBACK "Whether to allow dangless to gracefully handle running out of virtual memory and continue operating as a proxy to the underlying memory allocator." ON)

option(COLLECT_STATISTICS "Whether to collect statistics during runtime about Dangless usage. If enabled, statistics are printed after every run to stderr. These are only for local developer use and are not uploaded anywhere." ON)
option(REPORT_RUSAGE "Whether to report resource usage as measured by the kernel alongside the statistics at the end of each execution to stderr." ON)
option(ENABLE_PERF_EVENTS "Whether to enable hardware performance counters about the TLB. If enabled, these are printed alongside the statistics at the end of the each execution to stderr." ON)

option(AUTO_DEDICATE_PML4ES "Whether Dangless should automatically dedicate any unused PML4 pagetable entries (each representing 512 GB of virtual memory) for its virtual memory allocator. If disabled, user code will have to call dangless_dedicate_vmem()." ON)
set(CALLOC_SPECIAL_BUFSIZE 32 CACHE INTERNAL "Buffer size for special handling of the first calloc() call originating from dlsym().")
option(SYSCALLMETA_HAS_INFO "Whether to embed metadata information about system calls. If enabled, then system call logging will be more useful." ON)
set(PRINTF_NOMALLOC_BUFFER_SIZE 4096 CACHE STRING "Global buffer size for printf_nomalloc(). Printing a string longer than this will fail.")
set(DUNE_VMCALL_FIXUP_RESTORE_INITIAL_BUFFER_SIZE 32 CACHE STRING "Initial size of the vmcall fixup restoration buffer in which the address and original value of any nested pointers referenced in system call arguments is stored.")

option(DPRINTF_ENABLED "Whether to enable logging." ON)
option(DEBUG_DGLMALLOC "Debug mode for dangless_malloc.c" OFF)
option(DEBUG_VIRTMEM "Debug mode for virtmem.c" OFF)
option(DEBUG_VIRTMEMALLOC "Debug mode for virtmem_alloc.c" OFF)
option(DEBUG_VIRTREMAP "Debug mode for virtual_remap.c" OFF)
option(DEBUG_SYSMALLOC "Debug mode for sysmalloc.c" OFF)
option(DEBUG_INIT "Debug mode for init.c" OFF)
option(DEBUG_PHYSMEM_ALLOC "Debug mode for physmem_alloc.c" OFF)
option(DEBUG_DUNE_VMCALL_PREHOOK "Debug mode for vmcall_hooks.c" OFF)
option(DEBUG_DUNE_VMCALL_FIXUP "Debug mode for vmcall_fixup.c" OFF)
option(DEBUG_DUNE_VMCALL_FIXUP_RESTORE "Debug mode for vmcall_fixup_restore.c" OFF)
option(DEBUG_PERF_EVENTS "Debug mode for common/perf_events.c" OFF)

option(TRACE_HOOKS "Whether to log information whenever a Dangless memory handling function is called" OFF)
option(TRACE_HOOKS_BACKTRACE "Whether to attempt to use backtrace() and backtrace_symbols() to log information about the source of Dangless memory handling function calls" OFF)
set(TRACE_HOOKS_BACKTRACE_BUFSIZE 64 CACHE STRING "Buffer size for backtracing the source of Dangless memory handling function calls")

option(TRACE_SYSCALLS "Whether to log information about each system call. This information is much more useful with SYSCALLMETA_HAS_INFO enabled." OFF)
option(TRACE_SYSCALLS_NO_WRITE_STDOUT "Whether to ignore write() system calls to stdout." ON)
option(TRACE_SYSCALLS_NO_WRITE_STDERR "Whether to ignore write() system calls to stderr." ON)

option(SUPPORT_MULTITHREADING "Whether to enable multi-threading support. EXPERIMENTAL!" OFF)

# https://cmake.org/cmake/help/v3.5/command/target_link_libraries.html#libraries-for-both-a-target-and-its-dependents
cmake_policy(SET CMP0022 NEW)

set(DANGLESS_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(DANGLESS_BIN "dangless_malloc")

get_filename_component(LINUX_SYSCALLMETA_PATH "../vendor/linux-syscallmd" REALPATH)

list(APPEND CMAKE_MODULE_PATH
  "${CMAKE_SOURCE_DIR}/cmake/modules"
  "${LINUX_SYSCALLMETA_PATH}/cmake"
)

find_package(LibDune REQUIRED)

add_library(${DANGLESS_BIN} STATIC "")

target_include_directories(${DANGLESS_BIN}
  PUBLIC
    "${PROJECT_BINARY_DIR}/include"
    "${PROJECT_SOURCE_DIR}/include"
    "${LibDune_INCLUDE_DIRS}"
  PRIVATE
    "${PROJECT_SOURCE_DIR}/src"
)

target_compile_options(${DANGLESS_BIN}
  PRIVATE
    "-Wall"
    "-Wextra"
    "-Wno-format-zero-length"
    "-Wno-sign-compare"
    "-Wno-macro-redefined"
  INTERFACE
    "-no-pie"
)

target_link_libraries(${DANGLESS_BIN}
  PRIVATE
    "-ldl"
  PUBLIC
    Dune::LibDune
  INTERFACE
    "-no-pie"
)

if(${TRACE_HOOKS_BACKTRACE})
  # -rdynamic is required with the GNU linker to make symbol names available for backtrace_symbols() and backtrace_symbols_fd()
  # see: https://linux.die.net/man/3/backtrace_symbols
  target_link_libraries(${DANGLESS_BIN}
    INTERFACE
      "-rdynamic"
  )

  # used by the configured dangless_user.make.in
  set(USER_LDFLAGS_EXTRA "-rdynamic")
endif()

if(${SUPPORT_MULTITHREADING})
  target_compile_options(${DANGLESS_BIN}
    PUBLIC
      "-pthread"
  )

  target_link_libraries(${DANGLESS_BIN}
    INTERFACE
      "-pthread"
  )
endif()

set_target_properties(${DANGLESS_BIN}
  PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS ON
)

find_package(Python3 COMPONENTS Interpreter REQUIRED)
find_package(LinuxHeaders REQUIRED)

set(GENERATED_INCLUDE_DIR "${PROJECT_BINARY_DIR}/include/dangless/build")
file(MAKE_DIRECTORY "${GENERATED_INCLUDE_DIR}")

set(INC_SYSCALLMETA_PATH "${GENERATED_INCLUDE_DIR}/syscallmeta.inc")
set(INC_VMCALL_FIXUP_INFO_PATH "${GENERATED_INCLUDE_DIR}/vmcall_fixup_info.inc")

add_custom_command(
  OUTPUT "${INC_SYSCALLMETA_PATH}"
  COMMAND "${Python3_EXECUTABLE}" "${LINUX_SYSCALLMETA_PATH}/generate_c_header.py" "${LinuxHeaders_DIR}" > "${INC_SYSCALLMETA_PATH}"
)

add_custom_command(
  OUTPUT "${INC_VMCALL_FIXUP_INFO_PATH}"
  COMMAND "${Python3_EXECUTABLE}" "${PROJECT_SOURCE_DIR}/scripts/gen_vmcall_fixup_info.py" "${LinuxHeaders_DIR}" > "${INC_VMCALL_FIXUP_INFO_PATH}"
  DEPENDS "${PROJECT_SOURCE_DIR}/scripts/gen_vmcall_fixup_info.py"
)

# add the generated files as sources (even though they aren't really), otherwise they won't get generated
target_sources(${DANGLESS_BIN}
  PRIVATE
    "${INC_SYSCALLMETA_PATH}"
    "${INC_VMCALL_FIXUP_INFO_PATH}"
)

include("cmake/target_sources_local.cmake")
include("cmake/configure_file_generate.cmake")

configure_file_generate(
  "${PROJECT_SOURCE_DIR}/cmake/templates/dangless_user.make.in"
  "${PROJECT_BINARY_DIR}/dangless_user.make"
  @ONLY
)

configure_file_generate(
  "${PROJECT_SOURCE_DIR}/cmake/templates/buildconfig.h.in"
  "${PROJECT_BINARY_DIR}/include/dangless/build/config.h"
  @ONLY
)

add_subdirectory(src)

enable_testing()
add_subdirectory(tests)

add_subdirectory(testapps)

# TODO: install
# TODO: export targets
# see https://pabloariasal.github.io/2018/02/19/its-time-to-do-cmake-right/
