set(DUNE_ROOT "../vendor/dune-ix" CACHE PATH "Path to the Dune repository root. Defaults to ../vendor/dune-ix")

get_filename_component(DUNE_ROOT
  "${DUNE_ROOT}"
  REALPATH
  BASE_DIR "${PROJECT_SOURCE_DIR}"
)

if (NOT EXISTS "${DUNE_ROOT}/libdune")
  message("The DUNE_ROOT option must be set to a path to the Dune repository: '${DUNE_ROOT}/libdune' could not be found")
else()
  set(LibDune_DIR "${DUNE_ROOT}/libdune")
  set(LibDune_LIBRARY_NAME "dune")
  set(LibDune_LIBRARY_PATH "${LibDune_DIR}/libdune.a")
  set(LibDune_INCLUDE_DIR "${DUNE_ROOT}/libdune")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibDune
  REQUIRED_VARS
    LibDune_DIR
    LibDune_LIBRARY_NAME
    LibDune_LIBRARY_PATH
    LibDune_INCLUDE_DIR
)
