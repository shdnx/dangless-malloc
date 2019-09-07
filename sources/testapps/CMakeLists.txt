function(link_dangless_user TARGET)
  # TODO: maybe we use object libraries instead? See https://cmake.org/cmake/help/v3.9/manual/cmake-buildsystem.7.html#object-libraries

  target_link_libraries(${TARGET}
    PRIVATE
      "-Wl,--whole-archive"
      "${DANGLESS_BIN}"
      "-Wl,--no-whole-archive"
  )
endfunction()

function(simple_c_testapp NAME)
  add_executable(${NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.c"
  )

  link_dangless_user(${NAME})

  set_target_properties(${NAME}
    PROPERTIES
      C_STANDARD 11
      C_EXTENSIONS ON
  )
endfunction()

function(simple_cxx_testapp NAME)
  add_executable(${NAME}
    "${CMAKE_CURRENT_SOURCE_DIR}/${NAME}.cpp"
  )

  link_dangless_user(${NAME})

  set_target_properties(${NAME}
    PROPERTIES
      CXX_STANDARD 11
      CXX_EXTENSIONS ON
      LINKER_LANGUAGE CXX
  )
endfunction()

add_subdirectory(clone)
add_subdirectory(cpp-ofstream)
add_subdirectory(dune-mem-layout)
add_subdirectory(hello-world)
add_subdirectory(memstress)
add_subdirectory(writev-syscall)
