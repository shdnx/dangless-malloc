cmake_minimum_required(VERSION 3.5)
project(dune_autoenter LANGUAGES C)

get_filename_component(DANGLESS_ROOT
  "../"
  REALPATH
  BASE_DIR "${CMAKE_CURRENT_LIST_DIR}"
)

list(APPEND CMAKE_MODULE_PATH
  "${DANGLESS_ROOT}/sources/cmake/modules"
)

find_package(LibDune REQUIRED)

set(LIB "dune-autoenter")

configure_file(
  "${PROJECT_SOURCE_DIR}/user.make.in"
  "${PROJECT_BINARY_DIR}/user.make"
  @ONLY
)

add_library(${LIB} STATIC
  dune-autoenter.c
)

target_compile_options(${LIB}
  PRIVATE
    "-Wall"
    "-Wextra"
)

set_target_properties(${LIB}
  PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS ON
)

target_link_libraries(${LIB}
  PUBLIC
    Dune::LibDune
)
