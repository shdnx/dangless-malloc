add_executable(dune-mem-layout
  virtmem_chart_layout.c
  virtmem_chart_layout.h
  dune-mem-layout.c
)

link_dangless_user(dune-mem-layout)

set_target_properties(dune-mem-layout
  PROPERTIES
    C_STANDARD 11
    C_EXTENSIONS ON
)
