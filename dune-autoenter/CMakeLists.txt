cmake_minimum_required(VERSION 2.6)
project(dune_autoenter VERSION 1.0 LANGUAGES C)

set(DUNE_ROOT "" CACHE PATH "Absolute path to the Dune repository. Required.")
if (NOT ${DUNE_ROOT})
  message(FATAL_ERROR "The DUNE_ROOT option must be set to the absolute path to the Dune repository")
endif()

set(BIN "dune-autoenter")

configure_file(
  "${PROJECT_SOURCE_DIR}/user.make.in"
  "${PROJECT_BINARY_DIR}/user.make"
)

add_library(${BIN}
  dune-autoenter.c
)

target_include_directories(${BIN}
  PRIVATE
    "${DUNE_ROOT}/libdune"
)

target_compile_options(${BIN}
  "-Wall"
  "-Wextra"
  "-pthread"
)

target_compile_features(${BIN} PUBLIC c_std_11)
