#include <stdarg.h>
#include <limits.h>
#include <unistd.h>

#include "common.h"
#include "printf_nomalloc.h"

#define BUFFER_SIZE 1024

static char g_buffer[BUFFER_SIZE];

static int translate_file(FILE *f) {
  if (f == stdout) return 1;
  else if (f == stderr) return 2;
  else UNREACHABLE("Bad file descriptor: only stdout and stderr are supported!");
}

static size_t print_int(char *dest, i64 num, int base) {
  ASSERT(num != LLONG_MIN, "Implementation limitation");

  size_t nchars = 0;

  if (num < 0) {
    if (base == 10) {
      // TODO: this will not work for LLONG_MIN
      num *= -1;
      *dest++ = '-';
      nchars++;
    } else {
      // print as unsigned
      // TODO: ??
      num *= -1;
    }
  }

  // move the pointer to where the number will end
  i64 i = num;
  do {
    dest++;
    i /= base;
  } while (i > 0);

  // write the digits from the end backwards
  do {
    *(--dest) = "0123456789ABCDEF"[num % base];
    nchars++;
    num /= base;
  } while (num > 0);

  return nchars;
}

static size_t print_uint(char *dest, u64 num, int base) {
  size_t nchars = 0;

  // move the pointer to where the number will end
  u64 i = num;
  do {
    dest++;
    i /= base;
  } while (i > 0);

  // write the digits from the end backwards
  do {
    *(--dest) = "0123456789ABCDEF"[num % base];
    nchars++;
    num /= base;
  } while (num > 0);

  return nchars;
}

struct format_modifiers {
  int longness; // 0 for "%d", 1 for "%ld", 2 for "%lld"
};

#define FORMAT_MODIFIERS_INIT { 0 }

static i64 fetch_int_arg(struct format_modifiers *modifiers, va_list args) {
  if (modifiers->longness == 2) return (i64)va_arg(args, long long int);
  if (modifiers->longness == 1) return (i64)va_arg(args, long int);
  else return (i64)va_arg(args, int);
}

static u64 fetch_uint_arg(struct format_modifiers *modifiers, va_list args) {
  if (modifiers->longness == 2) return (u64)va_arg(args, unsigned long long int);
  if (modifiers->longness == 1) return (u64)va_arg(args, unsigned long int);
  else return (u64)va_arg(args, unsigned int);
}

static size_t handle_format(
    char *restrict dest,
    char specifier,
    struct format_modifiers *modifiers,
    va_list args) {
  char *original_dest = dest;

  switch (specifier) {
    case 'd':
    case 'i':
      dest += print_int(dest, fetch_int_arg(modifiers, args), 10);
      break;

    case 'u':
      dest += print_uint(dest, fetch_uint_arg(modifiers, args), 10);
      break;

    case 'x':
    case 'X':
      dest += print_uint(dest, fetch_uint_arg(modifiers, args), 16);
      break;

    case 'p':
      *dest++ = '0';
      *dest++ = 'x';
      dest += print_uint(dest, (u64)va_arg(args, void *), 16);
      break;

    case 'c':
      *dest++ = (char)va_arg(args, int);
      break;

    case 's': {
      const char *restrict str_ptr = va_arg(args, const char *restrict);
      for (; *str_ptr; str_ptr++, dest++) {
        *dest = *str_ptr;
      }

      break;
    }

    case '%':
      *dest++ = '%';
      break;

    default:
      UNREACHABLE("Unimplemented format specifier: '%c'\n", specifier);
  }

  return dest - original_dest;
}

static char *do_vsprintf(const char *restrict format, va_list args, OUT size_t *plen) {
  const char *restrict format_ptr;
  char *restrict buffer_ptr = g_buffer;

  for (format_ptr = format; *format_ptr; format_ptr++) {
    char c = *format_ptr;
    if (c != '%') {
      *buffer_ptr++ = c;
      continue;
    }

    struct format_modifiers mod = FORMAT_MODIFIERS_INIT;

    c = *(++format_ptr);

    switch (c) {
    case 'l':
      mod.longness++;
      c = *(++format_ptr);
      break;

    case 'z':
      mod.longness = 2;
      c = *(++format_ptr);
      break;
    }

    buffer_ptr += handle_format(buffer_ptr, c, &mod, args);
  }

  // null terminator
  *buffer_ptr = 0;

  if (plen)
    OUT *plen = buffer_ptr - g_buffer;

  return g_buffer;
}

char *vsprintf_nomalloc(const char *restrict format, va_list args) {
  return do_vsprintf(format, args, NULL);
}

char *sprintf_nomalloc(const char *restrict format, ...) {
  va_list args;
  va_start(args, format);
  char *result = vsprintf_nomalloc(format, args);
  va_end(args);
  return result;
}

void vfprintf_nomalloc(FILE *os, const char *restrict format, va_list args) {
  int fd = translate_file(os);

  size_t len;
  char *str = do_vsprintf(format, args, OUT &len);

  ssize_t result = write(fd, str, len);
  ASSERT(result > 0, "Failed to write output to screen, write() returned code %d!", (int)result);
}

void fprintf_nomalloc(FILE *os, const char *restrict format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf_nomalloc(os, format, args);
  va_end(args);
}
