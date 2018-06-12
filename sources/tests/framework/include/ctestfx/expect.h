#ifndef CTESTFX_EXPECT_H
#define CTESTFX_EXPECT_H

#include <stdbool.h>
#include <string.h> // strcmp

#define _CTESTFX_SUBST_FORMATSPEC(PRE, VAL, ...) \
  _Generic((VAL), \
    bool: PRE "%d" __VA_ARGS__, \
    char: PRE "%c" __VA_ARGS__, \
    unsigned char: PRE "%hhu" __VA_ARGS__, \
    short: PRE "%hd" __VA_ARGS__, \
    int: PRE "%d" __VA_ARGS__, \
    long: PRE "%ld" __VA_ARGS__, \
    long long: PRE "%lld" __VA_ARGS__, \
    unsigned short: PRE "%hu" __VA_ARGS__, \
    unsigned int: PRE "%u" __VA_ARGS__, \
    unsigned long: PRE "%lu" __VA_ARGS__, \
    unsigned long long: PRE "%llu" __VA_ARGS__, \
    char *: PRE "\"%s\"" __VA_ARGS__, \
    const char *: PRE "\"%s\"" __VA_ARGS__, \
    void *: PRE "%p" __VA_ARGS__, \
    const void *: PRE "%p" __VA_ARGS__, \
    void (*)(): PRE "%p" __VA_ARGS__ \
  )

void _ctestfx_expect_fail(const char *func, int line, const char *message, ...);

#define _CTESTFX_EXPECT_FAIL(...) \
  _ctestfx_expect_fail(__func__, __LINE__, __VA_ARGS__)

#define CTESTFX_EXPECT1(A, COND, MESSAGE) \
  do { \
    typeof((A)) _result = (A); \
    if (!COND(_result)) { \
      _CTESTFX_EXPECT_FAIL(_CTESTFX_SUBST_FORMATSPEC("%s\n\t%s = ", _result, "\n", MESSAGE, #A, _result)); \
      return; \
    } \
  } while (0)

#define CTESTFX_EXPECT2(A, B, COND, MESSAGE) \
  do { \
    typeof((A)) _result_a = (A); \
    typeof((B)) _result_b = (B); \
    if (!COND(_result_a, _result_b)) { \
      _CTESTFX_EXPECT_FAIL(_CTESTFX_SUBST_FORMATSPEC(_CTESTFX_SUBST_FORMATSPEC("%s\n\t%s = ", _result_a, "\n\t%s = "), _result_b, "\n", MESSAGE, #A, _result_a, #B, _result_b)); \
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
