# as of CMake 3.12, there's a FindPython3 built-in; this is to support older versions

find_program(Python3_EXECUTABLE
  NAMES
    python3.9
    python3.8
    python3.7
    python3.6
    python3
  PATHS
    /usr/bin
    /usr/local/bin
    /usr/pkg/bin
  NO_CMAKE_PATH
  NO_CMAKE_ENVIRONMENT_PATH
  NO_CMAKE_SYSTEM_PATH
)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(Python3
  REQUIRED_VARS Python3_EXECUTABLE
)
