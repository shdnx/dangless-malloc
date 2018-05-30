#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void fprintf_nomalloc(FILE *os, const char *restrict format, ...);
#define LOG(...) fprintf_nomalloc(stdout, __VA_ARGS__)

// A better source is include/virtmem.h
static inline int in_kernel_mode(void) {
  unsigned short cs;
  asm("mov %%cs, %0" : "=r" (cs));
  return (cs & 0x3) == 0x0;
}

extern void dangless_init(void);

int main(int argc, const char **argv) {
  if (puts("Hello world from userspace!") == EOF)
    perror("userspace puts()");

  dangless_init();

  if (!in_kernel_mode()) {
    fprintf(stderr, "Not in kernel mode, cannot hello world!!\n");
    return 1;
  }

  // ------

  puts("Hello world normally!");

  char *text = malloc(32);
  strcpy(text, "Hello malloc-d world!");

  LOG("text = [%p] \"%s\"\n", text, text);

  if (puts(text) == EOF) {
    perror("puts");
    LOG("puts failed!\n");
  }

  free(text);

  // -------

  char *filename = malloc(16);
  strcpy(filename, "./test.txt");

  LOG("filename = [%p] \"%s\"\n", filename, filename);

  FILE *fp = fopen(filename, "w");
  if (!fp) {
    perror("fopen");
    LOG("fopen failed!\n");
    return 1;
  }

  fwrite(filename, sizeof(char), strlen(filename), fp);
  fclose(fp);

  free(filename);

  LOG("filetest done\n");

  return 0;
}
