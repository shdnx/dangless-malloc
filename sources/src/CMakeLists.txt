target_sources_local(${DANGLESS_BIN}
  PRIVATE
    dangless_malloc.c
    dump.c
    syscallmeta.c
    virtmem.c
    virtmem_alloc.c
)

add_subdirectory(common)
add_subdirectory(platform)
