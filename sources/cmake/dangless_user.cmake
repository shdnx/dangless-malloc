# NOTE: this only works for within the Dangless tree

function(link_dangless_user TARGET)
  # TODO: maybe we use object libraries instead? See https://cmake.org/cmake/help/v3.9/manual/cmake-buildsystem.7.html#object-libraries

  target_link_libraries(${TARGET}
    PRIVATE
      "-Wl,--whole-archive"
      "${DANGLESS_BIN}"
      "-Wl,--no-whole-archive"
  )
endfunction()
