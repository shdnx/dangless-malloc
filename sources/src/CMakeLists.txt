target_sources(${DANGLESS_BIN}
  dangless_malloc.c
  dump.c
  syscallmeta.c
  virtmem.c
  virtmem_alloc.c
)

add_subdirectory(common)
add_subdirectory(platform)
