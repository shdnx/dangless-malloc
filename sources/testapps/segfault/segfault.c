#include <stdio.h>
#include <signal.h>

int main() {
  fprintf(stderr, "Going to trigger a segfault...\n");

  fprintf(stderr, *(int *)0);
  //raise(11); // SIGSEGV

  return 0;
}
