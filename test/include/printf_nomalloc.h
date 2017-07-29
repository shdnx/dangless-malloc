#ifndef DANGLESS_PRINTF_NOMALLOC
#define DANGLESS_PRINTF_NOMALLOC

#include <stdio.h> // FILE *
#include <stdarg.h> // va_list

// Simple printf() and friends implementation that doesn't use dynamic memory allocation.
// Limitations:
//  - only supports stdout and stderr
//  - only recognises format specifiers: %d, %i, %u, %x, %X, %p, %c, %s, %%
//  - only handles modifiers: l and z for d, i, u and x

// NOTE: storage is empheremal and will be invalidated by the next call to any of these functions.
char *vsprintf_nomalloc(const char *restrict format, va_list args) __attribute__((format(printf, 1, 0)));
char *sprintf_nomalloc(const char *restrict format, ...) __attribute__((format(printf, 1, 2)));

void vfprintf_nomalloc(FILE *os, const char *restrict format, va_list args) __attribute__((format(printf, 2, 0)));
void fprintf_nomalloc(FILE *os, const char *restrict format, ...) __attribute__((format(printf, 2, 3)));

#endif
