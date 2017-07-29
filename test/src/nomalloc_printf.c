#include <stdarg.h>
#include <limits.h>
#include <unistd.h>

#include "common.h"
#include "nomalloc_printf.h"

#define BUFFER_SIZE 1024

static char g_buffer[BUFFER_SIZE];

static int translate_file(FILE *f) {
  if (f == stdout) return 1;
  else if (f == stderr) return 2;
  else UNREACHABLE("Bad file descriptor: only stdout and stderr are supported!");
}

static size_t print_num(char *dest, i64 num, int base) {
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

  if (base == 16) {
    *dest++ = '0';
    *dest++ = 'x';
    nchars += 2;
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

static char *do_vsprintf(const char *restrict format, va_list args, OUT size_t *plen) {
  const char *restrict format_ptr;
  char *restrict buffer_ptr = g_buffer;

  for (format_ptr = format; *format_ptr; format_ptr++) {
    char c = *format_ptr;
    if (c != '%') {
      *buffer_ptr++ = c;
      continue;
    }

    c = *(++format_ptr);

#define HANDLE_INT(CHR, TYPE, BASE) \
    case CHR: \
      buffer_ptr += print_num(buffer_ptr, (i64)va_arg(args, TYPE), BASE); \
      break

    switch (c) {
      HANDLE_INT('d', int, 10);
      HANDLE_INT('u', unsigned, 10);
      HANDLE_INT('x', unsigned, 16);
      HANDLE_INT('p', void *, 16);

      case 'c':
        *buffer_ptr++ = (char)va_arg(args, int);
        break;

      case 's': {
        const char *restrict str_ptr = va_arg(args, const char *restrict);
        for (; *str_ptr; str_ptr++, buffer_ptr++) {
          *buffer_ptr = *str_ptr;
        }

        break;
      }

      case '%':
        *buffer_ptr++ = '%';
        break;

      default:
        UNREACHABLE("Unimplemented format specifier: '%c'\n", c);
    }
  }

  // null terminator
  *buffer_ptr++ = 0;

  if (plen)
    OUT *plen = buffer_ptr - g_buffer;

  return g_buffer;
}

char *nomalloc_vsprintf(const char *restrict format, va_list args) {
  return do_vsprintf(format, args, NULL);
}

char *nomalloc_sprintf(const char *restrict format, ...) {
  va_list args;
  va_start(args, format);
  char *result = nomalloc_vsprintf(format, args);
  va_end(args);
  return result;
}

void nomalloc_vfprintf(FILE *os, const char *restrict format, va_list args) {
  int fd = translate_file(os);

  size_t len;
  char *str = do_vsprintf(format, args, OUT &len);

  ssize_t result = write(fd, str, len);
  ASSERT(result > 0, "Failed to write output to screen, write() returned code %d!", (int)result);
}

void nomalloc_fprintf(FILE *os, const char *restrict format, ...) {
  va_list args;
  va_start(args, format);
  nomalloc_vfprintf(os, format, args);
  va_end(args);
}
