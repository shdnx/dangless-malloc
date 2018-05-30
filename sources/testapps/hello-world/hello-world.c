#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void fprintf_nomalloc(FILE *os, const char *restrict format, ...);
#define LOG(...) fprintf_nomalloc(stdout, __VA_ARGS__)

// A better source is include/virtmem.h
static inline int in_kernel_mode(void) {
  unsigned short cs;
  asm("mov %%cs, %0" : "=r" (cs));
  return (cs & 0x3) == 0x0;
}

int main(int argc, const char **argv) {
  if (!in_kernel_mode()) {
    fprintf(stderr, "Not in kernel mode, cannot hello world!!\n");
    return 1;
  }

  /*char *filename = malloc(16);
  strcpy(filename, "./test.txt");

  LOG("filename = [%p] \"%s\"\n", filename, filename);

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    perror("fopen");
    return 1;
  }

  fwrite(filename, sizeof(char), strlen(filename), fp);
  fclose(fp);

  free(filename);

  LOG("file OK\n");*/

  // --

  char *text = malloc(32);
  strcpy(text, "Hello world!\n");

  LOG("text = [%p] \"%s\"\n", text, text);

  int rc = puts(text);
  if (rc == EOF)
    perror("puts");

  free(text);

  return 0;
}
