#include <stdio.h>
#include <stdarg.h>

// this segfaults when compiled with GCC 5.4.0, but not e.g. with Clang 3.8.0
void bar_split(va_list args) {
  printf("bar: %d\n", va_arg(args, int));
  printf("bar: %s\n", va_arg(args, const char *));
}

// this works
void bar_split_var(va_list args) {
  int n = va_arg(args, int);
  printf("bar: %d\n", n);

  const char *s = va_arg(args, const char *);
  printf("bar: %s\n", s);
}

// this works
void bar_unified(va_list args) {
  printf("bar: %d\nbar: %s\n", (va_arg(args, int)), (va_arg(args, const char *)));
}

// this also works
void bar_unified_var(va_list args) {
  int n = va_arg(args, int);
  const char *s = va_arg(args, const char *);
  printf("bar: %d\nbar: %s\n", n, s);
}

void foo(int argc, ...) {
  va_list args;
  va_start(args, argc);

  if (argc > 1) {
    bar_split(args);
  } else {
    bar_unified(args);
  }

  printf("foo: %d\n", va_arg(args, int));
  printf("foo: %s\n", va_arg(args, const char *));

  va_end(args);
}

int main(int argc, const char **argv) {
  foo(argc, 1, "abc", 2, "xyz");
  return 0;
}
