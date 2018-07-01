#define _GNU_SOURCE

#include <stdio.h> // fputs
#include <stdlib.h> // abort
#include <string.h> // memcpy
#include <signal.h> // raise, SIGINT
#include <stdarg.h>
#include <errno.h> // perror
#include <limits.h>
#include <unistd.h>

#include "dangless/config.h"
#include "dangless/common/types.h"
#include "dangless/common/util.h"
#include "dangless/common/printf_nomalloc.h"

// The Bible: http://en.cppreference.com/w/c/io/fprintf

// NOTE: we cannot use dangless/common/dprintf.h nor dangless/common/assert.h because we're risking infinite recursion
// fun fact, we also can't use <assert.h> because on failure it may call malloc()

#define SAFE_ASSERT(COND, MSG) \
  if (UNLIKELY(!(COND))) { \
    fputs(__FILE__ ":" STRINGIFY(__LINE__) ": assertion '" #COND "' failed: " MSG "\n", stderr); \
    raise(SIGINT); \
    abort(); \
  }

#define SAFE_UNREACHABLE(MSG) \
  do { \
    fputs(__FILE__ ":" STRINGIFY(__LINE__) ": unreachable executed: " MSG "\n", stderr); \
    abort(); \
  } while (0)

static char g_buffer[DANGLESS_CONFIG_PRINTF_NOMALLOC_BUFFER_SIZE];

static int translate_file(FILE *f) {
  if (f == stdout) return 1;
  else if (f == stderr) return 2;
  else SAFE_UNREACHABLE("Bad file descriptor: only stdout and stderr are supported!");
}

static size_t print_str(char *dest, const char *src) {
  char *const dest_org = dest;

  while (*src) {
    *dest++ = *src++;
  }

  return dest - dest_org;
}

static size_t print_int(char *dest, i64 num, int base) {
  SAFE_ASSERT(num != LLONG_MIN, "Implementation limitation, cannot print LLONG_MIN");

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

enum {
  MIN_FIELD_WIDTH_CUSTOM = -1,

  JUSTIFY_RIGHT = 0,
  JUSTIFY_LEFT = 1
};

struct format_modifiers {
  int longness; // 0 for "%d", 1 for "%ld", 2 for "%lld"
  int min_field_width; // MIN_FIELD_WIDTH_CUSTOM for "*"
  int justify;
  char padding_char;
};

#define FORMAT_MODIFIERS_INIT \
  { \
    /*longness=*/0, \
    /*min_field_width=*/0, \
    /*justify=*/JUSTIFY_RIGHT, \
    /*padding_char=*/' ' \
  }

static i64 fetch_int_arg(struct format_modifiers *modifiers, va_list args) {
  switch (modifiers->longness) {
  case 2: return (i64)va_arg(args, long long int);
  case 1: return (i64)va_arg(args, long int);
  default: return (i64)va_arg(args, int);
  // short and char are not allowed, it breaks the ABI
  }
}

static u64 fetch_uint_arg(struct format_modifiers *modifiers, va_list args) {
  switch (modifiers->longness) {
  case 2: return (u64)va_arg(args, unsigned long long int);
  case 1: return (u64)va_arg(args, unsigned long int);
  default: return (u64)va_arg(args, unsigned int);
  // short and char are not allowed, it breaks the ABI
  }
}

static size_t handle_format(
    char *restrict dest,
    char specifier,
    struct format_modifiers *modifiers,
    va_list args) {
  // NOTE: due to 'restrict' we cannot use original_dest for writing, but we still need it for length determining
  char *const original_dest = dest;

  if (modifiers->min_field_width == MIN_FIELD_WIDTH_CUSTOM) {
    modifiers->min_field_width = va_arg(args, int);
    SAFE_ASSERT(modifiers->min_field_width >= 0, "Invalid custom minimum field width passed as argument!");
  }

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

    case 'p': {
      void *arg_ptr = va_arg(args, void *);

      if (arg_ptr) {
        dest += print_str(dest, "0x");
        dest += print_uint(dest, (u64)arg_ptr, 16);
      } else {
        dest += print_str(dest, "(null ptr)");
      }
      break;
    }

    case 'c':
      // note: va_arg(args, char) makes no sense and will cause a segmentation fault - integer promotion happens
      *dest++ = (char)va_arg(args, int);
      break;

    case 's': {
      const char *const arg_str = va_arg(args, char *);
      if (arg_str) {
        dest += print_str(dest, arg_str);
      } else {
        dest += print_str(dest, "(null str)");
      }
      break;
    }

    case '%':
      *dest++ = '%';
      break;

    default:
      SAFE_UNREACHABLE("Unimplemented format specifier!\n");
  }

  const size_t unpadded_len = dest - original_dest;
  if (unpadded_len < modifiers->min_field_width) {
    const size_t padding_len = modifiers->min_field_width - unpadded_len;

    if (modifiers->justify == JUSTIFY_LEFT) {
      for (size_t i = 0; i < padding_len; i++) {
        // always pad with space: even if the padding char '0' was requested, it's supposed to be ignored when justified to the left
        *dest++ = ' ';
      }
    } else { // JUSTIFY_RIGHT
      // everything we wrote, we have to undo to write the padding first; so move it all to a temporary buffer and re-write it after the padding
      dest -= unpadded_len;

      char tmp_buff[unpadded_len];
      memcpy(tmp_buff, dest, unpadded_len);

      for (size_t i = 0; i < padding_len; i++) {
        *dest++ = modifiers->padding_char;
      }

      memcpy(dest, tmp_buff, unpadded_len);
      dest += unpadded_len;
    }
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

    bool done_parsing_modifiers = false;
    while (!done_parsing_modifiers) {
      c = *(++format_ptr);

      switch (c) {
      case 'l':
        mod.longness++;
        break;

      case 'z':
        mod.longness = 2;
        break;

      case '-':
        mod.justify = JUSTIFY_LEFT;
        break;

      // can be part of the minimum field width, or can indicate that the padding character to use should be '0' instead of space (' ')
      case '0':
        if (mod.min_field_width == 0) {
          mod.padding_char = '0';
          break;
        }
        // deliberate fallthrough

      // case range (GCC extension)
      case '1' ... '9':
        SAFE_ASSERT(mod.min_field_width != MIN_FIELD_WIDTH_CUSTOM, "Both custom and explicit minimum field width specifier??");
        mod.min_field_width = mod.min_field_width * 10 + (c - '0');
        break;

      case '*':
        SAFE_ASSERT(mod.min_field_width == 0, "Both custom and explicit minimum field width specifier??");
        mod.min_field_width = MIN_FIELD_WIDTH_CUSTOM;
        break;

      default:
        done_parsing_modifiers = true;
        break;
      }
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

  if (result < 0) {
    perror("vfprintf_nomalloc: write()");
    SAFE_ASSERT(result >= 0, "write() failed!");
  }
}

void fprintf_nomalloc(FILE *os, const char *restrict format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf_nomalloc(os, format, args);
  va_end(args);
}
