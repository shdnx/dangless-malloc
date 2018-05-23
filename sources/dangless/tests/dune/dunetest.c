#include "dunetest.h"

/*static uintptr_t g_checking_memregion_start = 0;
static uintptr_t g_checking_memregion_end = 0;

void dunetest_check_memregion(void *start, size_t len) {
  g_checking_memregion_start = (uintptr_t)start;
  g_checking_memregion_end = (uintptr_t)start + len;
}

static void pagefault_handler(uintptr_t addr, ?? err, struct dune_tf *tf) {
  if (g_checking_memregion_start <= addr && addr <= g_checking_memregion_end) {

  }
}*/

void dunetest_init(void) {
  static bool s_initalized = false;
  if (s_initalized)
    return;

  s_initalized = true;

  fprintf(stderr, "Entering Dune...\n");

  if (dune_init_and_enter() != 0) {
    // TODO: a more elegant solution to this
    fprintf(stderr, "Failed to enter Dune mode!\n");
    abort();
  }

  //dune_register_pgflt_handler(&pagefault_handler);

  fprintf(stderr, "Now in Dune, starting tests...\n");
}
