include("${DANGLESS_ROOT}/cmake/dangless_user.cmake")

set(TEST_BIN "dangless-dune-tests")

add_executable(${TEST_BIN}
  dunetest.c
  basics.c
  common.c
  fileio.c
  math.c
  printf_nomalloc.c
  sysmalloc_fallback.c
  virtmem_alloc.c
)

link_dangless_user(${TEST_BIN})

target_link_libraries(${TEST_BIN}
  PRIVATE
    ctestfx
)

set_target_properties(${TEST_BIN}
  PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS ON
)

add_test(dune "${TEST_BIN}")

add_custom_target(
  vtest
  COMMAND "${TEST_BIN}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
