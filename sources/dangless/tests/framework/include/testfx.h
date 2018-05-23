#ifndef TESTFX_H
#define TESTFX_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h> // snprintf
#include <string.h> // strdup, strcmp
#include <stdlib.h> // free

#include "assert.h"

#define __DO_CONCAT2(A, B) A##B
#define CONCAT2(A, B) __DO_CONCAT2(A, B)

#define __DO_CONCAT3(A, B, C) A##B##C
#define CONCAT3(A, B, C) __DO_CONCAT3(A, B, C)

struct test_suite;
struct test_case;
struct test_case_result;

typedef void(*test_suite_func_t)(struct test_suite *);
typedef void(*test_case_func_t)(void);

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

extern struct test_suite *g_current_suite;
extern struct test_case *g_current_test;
extern struct test_case_result *g_current_result;

struct test_case_result *testcase_result_alloc(void);

void testsuite_register(struct test_suite *tsuite);

// Setup and teardown hooks: these hooks run before and after each test case respectively.
#define TEST_SETUP() void testcase_setup_hook(void)
#define TEST_TEARDOWN() void testcase_teardown_hook(void)

// Default, global implementations. Calls will resolve to these functions if there's no better (more specifically scoped, e.g. nested) function with the same name.
static inline TEST_SETUP() { /* empty */ }
static inline TEST_TEARDOWN() { /* empty */ }

bool testcase_prepare_run(
  struct test_suite *tsuite,
  struct test_case *tcase,
  /*OUT*/ struct test_case_result **presult
);

void testcase_register_run(
  struct test_suite *tsuite,
  struct test_case *tcase,
  struct test_case_result *result
);

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

#define _TEST_CASE_VARNAME(FNAME) CONCAT2(FNAME, _data
#define _TEST_CASE_DECL(FNAME) void FNAME (void)

// This mess is required because we cannot perform indirect calls to nested functions, as Dune doesn't like it - it causes a pagefault.
#define _TEST_CASE_IMPL(NAME, FNAME) \
  auto _TEST_CASE_DECL(FNAME); \
  { \
    struct test_case _TEST_CASE_VARNAME(FNAME) = { \
      /*name=*/NAME, \
      /*func=*/&FNAME \
    }; \
    struct test_case_result *_result = NULL; \
    if (testcase_prepare_run(_TEST_SUITE_PARMNAME, &_TEST_CASE_VARNAME(FNAME), &_result)) { \
      testcase_setup_hook(); \
      FNAME(); \
      testcase_teardown_hook(); \
      testcase_register_run(_TEST_SUITE_PARMNAME, &_TEST_CASE_VARNAME(FNAME), _result); \
    } \
  } \
  _TEST_CASE_DECL(FNAME)

#define TEST(NAME) \
  _TEST_CASE_IMPL(NAME, _TEST_CASE_FNAME_GEN())

#endif
