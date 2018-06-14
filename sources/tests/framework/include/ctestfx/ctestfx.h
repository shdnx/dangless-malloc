#ifndef CTESTFX_H
#define CTESTFX_H

#include "ctestfx/expect.h"

#define _CTESTFX_DO_CONCAT2(A, B) A##B
#define CTESTFX_CONCAT2(A, B) _CTESTFX_DO_CONCAT2(A, B)

#define _CTESTFX_DO_CONCAT3(A, B, C) A##B##C
#define CTESTFX_CONCAT3(A, B, C) _CTESTFX_DO_CONCAT3(A, B, C)

#define _CTESTFX_CTOR __attribute__((constructor))

struct test_case;
struct test_suite;
struct test_run_context;

typedef void(*test_case_func_t)(void);
typedef void(*test_suite_hook_t)(struct test_suite *);
typedef void(*test_case_hook_t)(struct test_run_context *);

enum test_enabled {
  TEST_ENABLED_DEFAULT = 0,
  TEST_ENABLED = 1,
  TEST_DISABLED = -1
};

enum test_case_status {
  TEST_CASE_SKIPPED = 0,
  TEST_CASE_PASSED = 1,
  TEST_CASE_FAILED = -1
};

struct test_case {
  const char *case_id;
  test_case_func_t func;

  enum test_enabled enabled;
  enum test_case_status status;

  const char *fail_func;
  int fail_line;
  /*owned*/ char *fail_message;

  struct test_case *next_case;
};

struct test_suite {
  const char *suite_id;
  const char *file;

  enum test_enabled enabled;

  test_suite_hook_t setup_hook;
  test_suite_hook_t teardown_hook;

  test_case_hook_t case_setup_hook;
  test_case_hook_t case_teardown_hook;

  size_t num_cases_total;
  size_t num_cases_run;
  size_t num_cases_failed;

  struct test_case *cases;
  struct test_suite *next_suite;
};

// TODO: with TEST_SUITE_CATEGORIES(...)
/*struct test_suite_category {
  const char *category_id;

  size_t num_suites;
  struct test_suite *suites;

  struct test_suite_category *next_category;
};*/

struct test_run_context {
  struct test_suite *tsuite;
  struct test_case *tcase;
};

void _testsuite_register(struct test_suite *tsuite);
void _testcase_register(struct test_case *tcase);

void _testsuite_register_setup(test_suite_hook_t func);
void _testsuite_register_teardown(test_suite_hook_t func);
void _testsuite_register_case_setup(test_case_hook_t func);
void _testsuite_register_case_teardown(test_case_hook_t func);

#define TEST_SUITE(SUITE_ID) \
  struct test_suite CTESTFX_CONCAT2(_testsuite_, SUITE_ID) = { \
    .suite_id = #SUITE_ID, \
    .file     = __FILE__ \
  }; \
  \
  _CTESTFX_CTOR \
  void CTESTFX_CONCAT2(_testsuite_register_, SUITE_ID)(void) { \
    _testsuite_register(&CTESTFX_CONCAT2(_testsuite_, SUITE_ID)); \
  }

#define _TEST_SUITE_HOOK_IMPL(FUNNAME, VARIANT, ...) \
  static void FUNNAME(__VA_ARGS__); \
  \
  _CTESTFX_CTOR \
  static void CTESTFX_CONCAT3(_, FUNNAME, _register)(void) { \
    CTESTFX_CONCAT2(_testsuite_register_, VARIANT)(&FUNNAME); \
  } \
  \
  static void FUNNAME(__VA_ARGS__)

#define TEST_SUITE_SETUP(...) \
  _TEST_SUITE_HOOK_IMPL(CTESTFX_CONCAT2(_testsuite_setup_, __COUNTER__), setup, __VA_ARGS__)

#define TEST_SUITE_TEARDOWN(...) \
  _TEST_SUITE_HOOK_IMPL(CTESTFX_CONCAT2(_testsuite_teardown_, __COUNTER__), teardown, __VA_ARGS__)

#define TEST_CASE(CASE_ID) \
  void CTESTFX_CONCAT2(testcase_, CASE_ID)(void); \
  \
  struct test_case CTESTFX_CONCAT2(_testcase_meta_, CASE_ID) = { \
    .case_id = #CASE_ID, \
    .func    = &CTESTFX_CONCAT2(testcase_, CASE_ID) \
  }; \
  \
  _CTESTFX_CTOR \
  void CTESTFX_CONCAT2(_testcase_register_, CASE_ID)(void) { \
    _testcase_register(&CTESTFX_CONCAT2(_testcase_meta_, CASE_ID)); \
  } \
  \
  void CTESTFX_CONCAT2(testcase_, CASE_ID)(void)

#define _TEST_CASE_HOOK_IMPL(FUNNAME, VARIANT, ...) \
  static void FUNNAME(__VA_ARGS__); \
  \
  _CTESTFX_CTOR \
  static void CTESTFX_CONCAT3(_, FUNNAME, _register)(void) { \
    CTESTFX_CONCAT2(_testsuite_register_case_, VARIANT)(&FUNNAME); \
  } \
  \
  static void FUNNAME(__VA_ARGS__)

#define TEST_CASE_SETUP(...) \
  _TEST_CASE_HOOK_IMPL(CTESTFX_CONCAT2(_testcase_setup_, __COUNTER__), setup, __VA_ARGS__)

#define TEST_CASE_TEARDOWN(...) \
  _TEST_CASE_HOOK_IMPL(CTESTFX_CONCAT2(_testcase_teardown_, __COUNTER__), teardown, __VA_ARGS__)

enum log_mode {
  LOG_NORMAL,
  LOG_VERBOSE
};

void _ctestfx_log(enum log_mode lmode, const char *file, int line, const char *format, ...) __attribute__((format(printf, 4, 5)));

#define _LOG_IMPL(MODE, ...) _ctestfx_log((MODE), __FILE__, __LINE__, __VA_ARGS__)

#define LOG(...) _LOG_IMPL(LOG_NORMAL, __VA_ARGS__)
#define LOG_VERBOSE(...) _LOG_IMPL(LOG_VERBOSE, __VA_ARGS__)
#define LOGV(...) LOG_VERBOSE(__VA_ARGS__)

#endif
