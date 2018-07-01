#define _GNU_SOURCE

#include <stdlib.h>
//#include <signal.h> // raise, SIGINT

void _dangless_assert_fail(void) {
  //raise(SIGINT);
  abort();
}
