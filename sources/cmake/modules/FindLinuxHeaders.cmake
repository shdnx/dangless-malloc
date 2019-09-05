execute_process(
  COMMAND uname -r
  OUTPUT_VARIABLE LINUX_VERSION
)

string(STRIP "${LINUX_VERSION}" LINUX_VERSION)

set(DIR "/usr/src/linux-headers-${LINUX_VERSION}")

if(EXISTS "${DIR}")
  set(LinuxHeaders_DIR "${DIR}")
else()
  message(WARN "Could not find linux headers in '${DIR}'")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LinuxHeaders
  REQUIRED_VARS LinuxHeaders_DIR
)
