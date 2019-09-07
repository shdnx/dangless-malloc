set(CTESTFX_ROOT "${CMAKE_CURRENT_LIST_DIR}")
set(CTESTFX_LIB_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(CTESTFX_LIB "ctestfx")

configure_file(
  "${CTESTFX_ROOT}/ctestfx_user.make.in"
  "${CTESTFX_LIB_DIR}/ctestfx_user.make"
  @ONLY
)

add_library(${CTESTFX_LIB}
  src/ctestfx.c
)

target_include_directories(${CTESTFX_LIB}
  PUBLIC
    "include"
)

target_compile_options(${CTESTFX_LIB}
  PRIVATE
    "-Wall"
    "-Wextra"
    "-Wno-unused-parameter"
)

set_target_properties(${CTESTFX_LIB}
  PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS ON
)
