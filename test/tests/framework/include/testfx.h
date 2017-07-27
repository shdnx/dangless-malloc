#ifndef TESTFX_H
#define TESTFX_H

#include <stdio.h>
#include <string.h> // strdup
#include <stdlib.h> // free

#include "common.h"

struct test_suite;
struct test_case;
struct test_case_result;

typedef void(*test_suite_func_t)(struct test_suite *);
typedef void(*test_case_func_t)(struct test_suite *, struct test_case *, struct test_case_result *);

struct test_case {
  char *name;
  test_case_func_t func;
};

struct test_case_result {
  char *case_name;

  bool pass;
  char *message;
  int line;

  struct test_case_result *next_result;
};

struct test_suite {
  char *file;
  char *name;
  test_suite_func_t func;

  size_t num_cases;
  size_t num_case_results;
  size_t num_failed_cases;
  struct test_case_result *case_results;
  struct test_suite *next_suite;
};

void testsuite_register(struct test_suite *tsuite);
void testcase_run(struct test_suite *tsuite, struct test_case *tcase);

#define _TEST_SUITE_REGISTER(FNAME) \
  __attribute__((constructor)) \
  static void CONCAT2(FNAME, _ctor) (void) { \
    testsuite_register(&_TEST_SUITE_GLOBNAME(FNAME)); \
  }

// --

#define _TEST_SUITE_GLOBNAME(FNAME) CONCAT3(_g_, FNAME, _data)
#define _TEST_SUITE_PARMNAME _tsuite

#define _TEST_SUITE_FNAME_GEN() CONCAT2(_testsuite_, __COUNTER__)

#define _TEST_SUITE_DECL(NAME) static void NAME(struct test_suite *_TEST_SUITE_PARMNAME)

#define _TEST_SUITE_IMPL(NAME, FNAME) \
  _TEST_SUITE_DECL(FNAME); \
  static struct test_suite _TEST_SUITE_GLOBNAME(FNAME) = { \
    /*file=*/__FILE__, \
    /*name=*/NAME, \
    /*func=*/&FNAME, \
    /*num_cases=*/0, \
    /*num_case_results=*/0, \
    /*num_failed_cases=*/0, \
    /*case_results=*/NULL, \
    /*next_suite=*/NULL \
  }; \
  _TEST_SUITE_REGISTER(FNAME) \
  _TEST_SUITE_DECL(FNAME)

#define TEST_SUITE(NAME) \
  _TEST_SUITE_IMPL(NAME, _TEST_SUITE_FNAME_GEN())

// --

#define _TEST_CASE_FNAME_GEN() CONCAT2(_testcase_, __COUNTER__)

#define _TEST_CASE_PARMNAME _tcase
#define _TEST_CASE_RESULT_PARMNAME _tcresult
#define _TEST_CASE_VARNAME(FNAME) CONCAT2(FNAME, _data)

#define _TEST_CASE_DECL(FNAME) \
  void FNAME ( \
    struct test_suite *_TEST_SUITE_PARMNAME, \
    struct test_case *_TEST_CASE_PARMNAME, \
    struct test_case_result *_TEST_CASE_RESULT_PARMNAME \
  )

#define _TEST_CASE_IMPL(NAME, FNAME) \
  auto _TEST_CASE_DECL(FNAME); \
  struct test_case _TEST_CASE_VARNAME(FNAME) = { \
    /*name=*/NAME, \
    /*func=*/&FNAME \
  }; \
  testcase_run(_TEST_SUITE_PARMNAME, &_TEST_CASE_VARNAME(FNAME)); \
  _TEST_CASE_DECL(FNAME)

#define TEST(NAME) \
  _TEST_CASE_IMPL(NAME, _TEST_CASE_FNAME_GEN())

// --

void _assert_fail(struct test_case *tc, struct test_case_result *tcr, int line, char *message);

#define _FMT(V) \
  _Generic((V), \
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
    void *: "%p" \
  )

#define SPRINTF(...) \
  ({ \
    int _sz = snprintf(NULL, 0, __VA_ARGS__); \
    char _buf[_sz + 1]; \
    snprintf(_buf, sizeof _buf, __VA_ARGS__); \
    strdup(_buf); \
  })

#define _ASSERT_FAIL(...) \
  _assert_fail(_TEST_CASE_PARMNAME, _TEST_CASE_RESULT_PARMNAME, __LINE__, SPRINTF(__VA_ARGS__));

#define _ASSERT1(A, COND, MESSAGE) \
  { \
    __typeof((A)) _result = (A); \
    if (!COND(_result)) { \
      char *fmt = SPRINTF("%%s\n\t%%s = %s\n", _FMT(_result)); \
      _ASSERT_FAIL(fmt, MESSAGE, #A, _result); \
      free(fmt); \
      return; \
    } \
  }

#define _ASSERT2(A, B, COND, MESSAGE) \
  { \
    __typeof((A)) _result_a = (A); \
    __typeof((B)) _result_b = (B); \
    if (!COND(_result_a, _result_b)) { \
      char *fmt = SPRINTF("%%s\n\t%%s = %s\n\t%%s = %s\n", _FMT(_result_a), _FMT(_result_b)); \
      _ASSERT_FAIL(fmt, MESSAGE, #A, _result_a, #B, _result_b); \
      free(fmt); \
      return; \
    } \
  }

#define _COND_EQ(AV, BV) ((AV) == (BV))
#define _COND_NOT_NULL(V) ((V) != NULL)

#define ASSERT_EQUALS(A, B) \
  _ASSERT2(A, B, _COND_EQ, "expected " #A " to equal " #B)

#define ASSERT_NOT_NULL(A) \
  _ASSERT1(A, _COND_NOT_NULL, "expected " #A " to be non-null");

#endif
