get_filename_component(DUNE_ROOT_DEFAULT
  "../../../vendor/dune-ix"
  REALPATH
  BASE_DIR "${CMAKE_CURRENT_LIST_DIR}"
)

set(DUNE_ROOT "${DUNE_ROOT_DEFAULT}" CACHE PATH "Path to the Dune repository root. Defaults to ${DUNE_ROOT_DEFAULT}")

if(NOT ${DUNE_ROOT} EQUAL ${DUNE_ROOT_DEFAULT})
  get_filename_component(DUNE_ROOT
    "${DUNE_ROOT}"
    REALPATH
    BASE_DIR "${PROJECT_SOURCE_DIR}"
  )
endif()

if (NOT EXISTS "${DUNE_ROOT}/libdune")
  message(FATAL_ERROR "Could not find package LibDune: '${DUNE_ROOT}/libdune' does not exist. Try setting the DUNE_ROOT variable, e.g. by passing -D DUNE_ROOT=path/to/my/dune to CMake")
else()
  set(LibDune_FOUND True)
  set(LibDune_DIR "${DUNE_ROOT}/libdune")
  set(LibDune_LIBRARY_NAME "dune")
  set(LibDune_LIBRARY_PATH "${LibDune_DIR}/libdune.a")
  set(LibDune_INCLUDE_DIRS "${DUNE_ROOT}/libdune")

  add_library(Dune::LibDune STATIC IMPORTED GLOBAL)
  set_target_properties(Dune::LibDune
    PROPERTIES
      IMPORTED_LOCATION "${LibDune_LIBRARY_PATH}"
      INTERFACE_INCLUDE_DIRECTORIES "${LibDune_INCLUDE_DIRS}"
      #PUBLIC_HEADER "${LibDune_INCLUDE_DIR}/libdune/dune.h"
      INTERFACE_COMPILE_OPTIONS "-pthread"
      INTERFACE_LINK_OPTIONS "-pthread"
      INTERFACE_LINK_LIBRARIES "dl"
  )

  message("-- Found LibDune: ${LibDune_DIR}")
endif()

# TODO: do we want this?
#include(FindPackageHandleStandardArgs)
# find_package_handle_standard_args(LibDune
#   REQUIRED_VARS
#     LibDune_DIR
#     LibDune_LIBRARY_NAME
#     LibDune_LIBRARY_PATH
#     LibDune_INCLUDE_DIR
# )
