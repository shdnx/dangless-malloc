#ifndef DANGLESS_COMMON_DPRINTF_H
#define DANGLESS_COMMON_DPRINTF_H

#ifdef NDEBUG
  #define dprintf(...) /* empty */
  #define dprintf_nomalloc(...) /* empty */

  #define dprintf_scope_enter(LABEL) /* empty */
  #define dprintf_scope_exit(LABEL) /* empty */

  #define vdprintf(...) /* empty */
  #define vdprintf_nomalloc(...) /* empty */
#else // !defined(NDEBUG)
  // we have to have stdio.h included BEFORE we define dprintf() and vdprintf(), because apparently those are function names in it, causing fun mayhem
  #include <stdio.h>

  extern int dprintf_scope_depth;

  #define dprintf_scope_enter(LABEL) (++dprintf_scope_depth)
  #define dprintf_scope_exit(LABEL) (--dprintf_scope_depth)

  void _dprintf(const char *restrict format, ...) __attribute__((format(printf, 1, 2)));
  void _dprintf_nomalloc(const char *restrict format, ...) __attribute__((format(printf, 1, 2)));

  #define dprintf(...) _dprintf(__VA_ARGS__)
  #define dprintf_nomalloc(...) _dprintf_nomalloc(__VA_ARGS__)

  void _print_caller_info(const char *file, const char *func, int line);
  void _print_caller_info_nomalloc(const char *file, const char *func, int line);

  // NOTE: this could be optimized, since we can turn all of these into string literals
  #define _CALLER_INFO_ARGS \
    __FILE__, __func__, __LINE__

  #define vdprintf(...) \
    do { \
      _print_caller_info(_CALLER_INFO_ARGS); \
      dprintf(__VA_ARGS__); \
    } while (0)

  #define vdprintf_nomalloc(...) \
    do { \
      _print_caller_info_nomalloc(_CALLER_INFO_ARGS); \
      dprintf_nomalloc(__VA_ARGS__); \
    } while (0)
#endif // !defined(NDEBUG)

#endif
