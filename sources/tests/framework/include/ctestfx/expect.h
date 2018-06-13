#ifndef CTESTFX_EXPECT_H
#define CTESTFX_EXPECT_H

#include <stdbool.h>
#include <string.h> // strcmp

#define _CTESTFX_STRINGIFY_IMPL(V) #V
#define _CTESTFX_STRINGIFY(V) _CTESTFX_STRINGIFY_IMPL(V)

#define _CTESTFX_FMT(V) \
  _Generic((V), \
    bool: "%d", \
    char: "%c", \
    unsigned char: "%hhu", \
    short: "%hd", \
    int: "%d", \
    long: "%ld", \
    long long: "%lld", \
    unsigned short: "%hu", \
    unsigned int: "%u", \
    unsigned long: "%lu", \
    unsigned long long: "%llu", \
    char *: "\"%s\"", \
    const char *: "\"%s\"", \
    void *: "%p", \
    const void *: "%p", \
    void (*)(): "%p" \
  )

// We cannot do the whole formatting with just one function call, because we run into a GCC 5.4.0 compiler bug with va_list. See vafwd-gcc-bug.c.
char *_ctestfx_preformat_fail_message(const char *format, ...);
void _ctestfx_expect_fail(const char *func, int line, const char *msg_format, ...);

#define _CTESTFX_EXPECT_FAIL(...) \
  _ctestfx_expect_fail(__func__, __LINE__, __VA_ARGS__)

#define CTESTFX_EXPECT1(A, COND, MESSAGE) \
  do { \
    typeof((A)) _result = (A); \
    if (!COND(_result)) { \
      char *_fmt = _ctestfx_preformat_fail_message("%s, but got:\n\t%s = %s\n", (MESSAGE), _CTESTFX_STRINGIFY(A), _CTESTFX_FMT(_result)); \
      _CTESTFX_EXPECT_FAIL(_fmt, _result); \
      return; \
    } \
  } while (0)

#define CTESTFX_EXPECT2(A, B, COND, MESSAGE) \
  do { \
    typeof((A)) _result_a = (A); \
    typeof((B)) _result_b = (B); \
    if (!COND(_result_a, _result_b)) { \
      char *_fmt = _ctestfx_preformat_fail_message("%s, but got:\n\t%s = %s\n\t%s = %s\n", (MESSAGE), _CTESTFX_STRINGIFY(A), _CTESTFX_FMT(_result_a), _CTESTFX_STRINGIFY(B), _CTESTFX_FMT(_result_b)); \
      _CTESTFX_EXPECT_FAIL(_fmt, _result_a, _result_b); \
      return; \
    } \
  } while (0)

#define _COND_TRUE(V) (!!(V))
#define _COND_FALSE(V) (!(V))
#define _COND_NULL(V) ((V) == NULL)
#define _COND_NOT_NULL(V) ((V) != NULL)

#define _COND_EQ(AV, BV) ((AV) == (BV))
#define _COND_EQ_STR(AV, BV) (strcmp((AV), (BV)) == 0)
#define _COND_NEQ(AV, BV) (!_COND_EQ(AV, BV))
#define _COND_NEQ_STR(AV, BV) (!_COND_EQ_STR(AV, BV))

#define EXPECT_TRUE(A) \
  CTESTFX_EXPECT1(A, _COND_TRUE, "expected " #A " to be true");

#define EXPECT_FALSE(A) \
  CTESTFX_EXPECT1(A, _COND_FALSE, "expected " #A " to be false");

#define EXPECT_NULL(A) \
  CTESTFX_EXPECT1(A, _COND_NULL, "expected " #A " to be null");

#define EXPECT_NOT_NULL(A) \
  CTESTFX_EXPECT1(A, _COND_NOT_NULL, "expected " #A " to be non-null");

#define EXPECT_EQUALS(A, B) \
  CTESTFX_EXPECT2(A, B, _COND_EQ, "expected " #A " to equal " #B)

#define EXPECT_EQUALS_STR(A, B) \
  CTESTFX_EXPECT2(A, B, _COND_EQ_STR, "expected string " #A " to equal " #B)

#define EXPECT_EQUALS_PTR(A, B) \
  CTESTFX_EXPECT2((void *)(A), (void *)(B), _COND_EQ, "expected pointer " #A " to equal " #B)

#define EXPECT_NOT_EQUALS(A, B) \
  CTESTFX_EXPECT2(A, B, _COND_NEQ, "expected " #A " to NOT equal " #B)

#define EXPECT_NOT_EQUALS_STR(A, B) \
  CTESTFX_EXPECT2(A, B, _COND_NEQ_STR, "expected string " #A " to NOT equal " #B)

#define EXPECT_NOT_EQUALS_PTR(A, B) \
  CTESTFX_EXPECT2((void *)(A), (void *)(B), _COND_NEQ, "expected pointer " #A " to NOT equal " #B)

#endif
