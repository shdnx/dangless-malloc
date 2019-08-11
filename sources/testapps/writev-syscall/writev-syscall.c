#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>

static void dump_iovec(const struct iovec *v, size_t len) {
  fprintf(stderr, "Dumping struct iovec at %p:\n", v);

  for (size_t i = 0; i < len; ++i) {
    fprintf(stderr, " [%zu]: %p with len=%zu\n", i, v[i].iov_base, v[i].iov_len);
  }
}

int main(int argc, const char *argv[]) {
  char *buffer1 = malloc(32 * sizeof(char));
  if (!buffer1) {
    fputs(stderr, "Failed to allocate buffer1!");
    return 1;
  }

  fprintf(stderr, "buffer1 = %p\n", buffer1);

  char *buffer2 = malloc(16 * sizeof(char));
  if (!buffer2) {
    fputs(stderr, "Failed to allocate buffer2!");
    return 1;
  }

  fprintf(stderr, "buffer2 = %p\n", buffer2);

  char *str_part1 = strcpy(buffer1, "Hello");
  char *str_part2 = strcpy(buffer1 + strlen(str_part1) + 1, " world");
  char *str_part3 = strcpy(buffer2, " again!\n");

  struct iovec iov[3];
  iov[0].iov_base = str_part1;
  iov[0].iov_len = strlen(str_part1);
  iov[1].iov_base = str_part2;
  iov[1].iov_len = strlen(str_part2);
  iov[2].iov_base = str_part3;
  iov[2].iov_len = strlen(str_part3);

  dump_iovec(iov, 3);

  ssize_t nwritten = writev(STDOUT_FILENO, iov, 3);
  fprintf(stderr, "Written %ld bytes\n", nwritten);

  dump_iovec(iov, 3);

  free(buffer2);
  free(buffer1);

  return 0;
}
