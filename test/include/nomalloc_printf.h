#ifndef DANGLESS_NOMALLOC_PRINTF
#define DANGLESS_NOMALLOC_PRINTF

#include <stdio.h>
#include <stdarg.h>

// Simple printf() and friends implementation that doesn't use dynamic memory allocation.
// Limitations:
//  - only supports stdout and stderr
//  - only recognises format specifiers: %d, %u, %x, %p, %c, %s, %%
//  - does not handle any modifiers

// NOTE: storage is empheremal and will be invalidated by the next call to any of these functions.
char *nomalloc_vsprintf(const char *restrict format, va_list args);
char *nomalloc_sprintf(const char *restrict format, ...);

void nomalloc_vfprintf(FILE *os, const char *restrict format, va_list args);
void nomalloc_fprintf(FILE *os, const char *restrict format, ...);

#endif
