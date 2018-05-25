#ifndef TESTFX_INTERNALS_TESTSUITE_H
#define TESTFX_INTERNALS_TESTSUITE_H

#include <stdbool.h>
#include <stddef.h>

#include "testfx/internals/common.h"

// Default, global implementations. Calls will resolve to these functions if there's no better (more specifically scoped, e.g. nested) function with the same name.
static inline TEST_SETUP() { /* empty */ }
static inline TEST_TEARDOWN() { /* empty */ }

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

#define _TEST_SUITE_REGISTER(FNAME) \
  __attribute__((constructor)) \
  static void TESTFX_CONCAT2(FNAME, _ctor) (void) { \
    testsuite_register(&_TEST_SUITE_GLOBNAME(FNAME)); \
  }

// --

#define _TEST_SUITE_GLOBNAME(FNAME) TESTFX_CONCAT3(_g_, FNAME, _data)
#define _TEST_SUITE_PARMNAME _tsuite

#define _TEST_SUITE_FNAME_GEN() TESTFX_CONCAT2(_testsuite_, __COUNTER__)

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

// --

#define _TEST_CASE_FNAME_GEN() TESTFX_CONCAT2(_testcase_, __COUNTER__)

#define _TEST_CASE_VARNAME(FNAME) TESTFX_CONCAT2(FNAME, _data)
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

#endif
