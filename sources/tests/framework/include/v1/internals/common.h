#ifndef TESTFX_INTERNALS_COMMON_H
#define TESTFX_INTERNALS_COMMON_H

#define _TESTFX_DO_CONCAT2(A, B) A##B
#define TESTFX_CONCAT2(A, B) _TESTFX_DO_CONCAT2(A, B)

#define _TESTFX_DO_CONCAT3(A, B, C) A##B##C
#define TESTFX_CONCAT3(A, B, C) _TESTFX_DO_CONCAT3(A, B, C)

struct test_suite;
struct test_case;
struct test_case_result;

extern struct test_suite *g_current_suite;
extern struct test_case *g_current_test;
extern struct test_case_result *g_current_result;

typedef void(*test_suite_func_t)(struct test_suite *);
typedef void(*test_case_func_t)(void);

struct test_case_result *testcase_result_alloc(void);

void testsuite_register(struct test_suite *tsuite);

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

#endif
