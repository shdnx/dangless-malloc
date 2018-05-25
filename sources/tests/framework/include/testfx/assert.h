#ifndef TESTFX_ASSERT_H
#define TESTFX_ASSERT_H

#include <stdio.h> // snprintf
#include <string.h> // strdup, strcmp
#include <stdlib.h> // free

#include "testfx/internals/common.h"

#define _FMT(V) \
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
    void *: "%p", \
    void (*)(): "%p" \
  )

#define SPRINTF(...) \
  ({ \
    int _sz = snprintf(NULL, 0, __VA_ARGS__); \
    char _buf[_sz + 1]; \
    snprintf(_buf, sizeof _buf, __VA_ARGS__); \
    strdup(_buf); \
  })

void _test_assert_fail(struct test_case *tc, struct test_case_result *tcr, int line, char *message);

#define _ASSERT_FAIL(...) \
  _test_assert_fail(g_current_test, g_current_result, __LINE__, SPRINTF(__VA_ARGS__));

#define _ASSERT1(A, COND, MESSAGE) \
  { \
    typeof((A)) _result = (A); \
    if (!COND(_result)) { \
      char *fmt = SPRINTF("%%s\n\t%%s = %s\n", _FMT(_result)); \
      _ASSERT_FAIL(fmt, MESSAGE, #A, _result); \
      free(fmt); \
      return; \
    } \
  }

#define _ASSERT2(A, B, COND, MESSAGE) \
  { \
    typeof((A)) _result_a = (A); \
    typeof((B)) _result_b = (B); \
    if (!COND(_result_a, _result_b)) { \
      char *fmt = SPRINTF("%%s\n\t%%s = %s\n\t%%s = %s\n", _FMT(_result_a), _FMT(_result_b)); \
      _ASSERT_FAIL(fmt, MESSAGE, #A, _result_a, #B, _result_b); \
      free(fmt); \
      return; \
    } \
  }

#define _COND_TRUE(V) (!!(V))
#define _COND_FALSE(V) (!(V))
#define _COND_NULL(V) ((V) == NULL)
#define _COND_NOT_NULL(V) ((V) != NULL)

#define _COND_EQ(AV, BV) ((AV) == (BV))
#define _COND_EQ_STR(AV, BV) (strcmp((AV), (BV)) == 0)
#define _COND_NEQ(AV, BV) (!_COND_EQ(AV, BV))
#define _COND_NEQ_STR(AV, BV) (!_COND_EQ_STR(AV, BV))

#define ASSERT_TRUE(A) \
  _ASSERT1(A, _COND_TRUE, "expected " #A " to be true");

#define ASSERT_FALSE(A) \
  _ASSERT1(A, _COND_FALSE, "expected " #A " to be false");

#define ASSERT_NULL(A) \
  _ASSERT1(A, _COND_NULL, "expected " #A " to be null");

#define ASSERT_NOT_NULL(A) \
  _ASSERT1(A, _COND_NOT_NULL, "expected " #A " to be non-null");

#define ASSERT_EQUALS(A, B) \
  _ASSERT2(A, B, _COND_EQ, "expected " #A " to equal " #B)

#define ASSERT_EQUALS_STR(A, B) \
  _ASSERT2(A, B, _COND_EQ_STR, "expected string " #A " to equal " #B)

#define ASSERT_EQUALS_PTR(A, B) \
  _ASSERT2((void *)(A), (void *)(B), _COND_EQ, "expected pointer " #A " to equal " #B)

#define ASSERT_NOT_EQUALS(A, B) \
  _ASSERT2(A, B, _COND_NEQ, "expected " #A " to NOT equal " #B)

#define ASSERT_NOT_EQUALS_STR(A, B) \
  _ASSERT2(A, B, _COND_NEQ_STR, "expected string " #A " to NOT equal " #B)

#define ASSERT_NOT_EQUALS_PTR(A, B) \
  _ASSERT2((void *)(A), (void *)(B), _COND_NEQ, "expected pointer " #A " to NOT equal " #B)

#endif
