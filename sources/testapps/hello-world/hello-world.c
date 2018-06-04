#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// dangless/printf_nomalloc.h
extern void fprintf_nomalloc(FILE *os, const char *restrict format, ...);
#define LOG(...) fprintf_nomalloc(stdout, __VA_ARGS__)

// dangless/virtmem.h
static inline int in_kernel_mode(void) {
  unsigned short cs;
  asm("mov %%cs, %0" : "=r" (cs));
  return (cs & 0x3) == 0x0;
}

int main(int argc, const char **argv) {
  if (!in_kernel_mode()) {
    LOG("Not in kernel mode, this would be a boring hello world!\n");
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
  } else {
    LOG("puts OK\n");
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
  } else {
    fwrite(filename, sizeof(char), strlen(filename), fp);
    fclose(fp);

    LOG("filetest OK\n");
  }

  free(filename);

  return 0;
}
