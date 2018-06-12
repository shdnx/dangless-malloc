#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "ctestfx/ctestfx.h"

static struct test_suite *g_last_testsuite;
static size_t g_num_testsuites_total;

static struct test_suite *g_current_suite;
static struct test_case *g_current_test;

#define LOG_DEBUG(...) fprintf(stderr, __VA_ARGS__)
#define LOG_VERBOSE(...) printf(__VA_ARGS__)

void _testsuite_register(struct test_suite *tsuite) {
  if (g_last_testsuite) {
    LOG_DEBUG("-- Done registering test suite %s: %zu test cases\n\n", g_last_testsuite->suite_id, g_last_testsuite->num_cases_total);
  }

  tsuite->next_suite = g_last_testsuite;
  g_last_testsuite = tsuite;

  g_num_testsuites_total++;

  LOG_DEBUG("-- Registering test suite %s from %s\n", tsuite->suite_id, tsuite->file);
}

void _testsuite_register_setup(test_suite_hook_t func) {
  g_last_testsuite->setup_hook = func;

  LOG_DEBUG("--- Registered setup hook %p for test suite %s in %s\n", func, g_last_testsuite->suite_id, g_last_testsuite->file);
}

void _testsuite_register_teardown(test_suite_hook_t func) {
  g_last_testsuite->teardown_hook = func;

  LOG_DEBUG("--- Registered teardown hook %p for test suite %s in %s\n", func, g_last_testsuite->suite_id, g_last_testsuite->file);
}

void _testsuite_register_case_setup(test_case_hook_t func) {
  g_last_testsuite->case_setup_hook = func;

  LOG_DEBUG("--- Registered case setup hook %p for test suite %s in %s\n", func, g_last_testsuite->suite_id, g_last_testsuite->file);
}

void _testsuite_register_case_teardown(test_case_hook_t func) {
  g_last_testsuite->case_teardown_hook = func;

  LOG_DEBUG("--- Registered case teardown hook %p for test suite %s in %s\n", func, g_last_testsuite->suite_id, g_last_testsuite->file);
}

void _testcase_register(struct test_case *tcase) {
  tcase->next_case = g_last_testsuite->cases;
  g_last_testsuite->cases = tcase;

  g_last_testsuite->num_cases_total++;

  LOG_DEBUG("--- Registered test case %s\n", tcase->case_id);
}

void _ctestfx_expect_fail(const char *func, int line, const char *msg_format, ...) {
  g_current_test->status = TEST_CASE_FAILED;

  g_current_test->fail_func = func;
  g_current_test->fail_line = line;

  va_list args1;
  va_start(args1, msg_format);

  // we need 2 copies of the argument list, because the first vsnprintf() call that is needed to compute the buffer size will consume args1
  va_list args2;
  va_copy(args2, args1);

  const int fail_msg_size = vsnprintf(NULL, 0, msg_format, args1);
  va_end(args1);

  g_current_test->fail_message = malloc((fail_msg_size + 1) * sizeof(char));
  if (!g_current_test->fail_message) {
    fprintf(stderr, "!!! Failed to record test failure: out of memory!\n");
    perror("malloc error: ");
    abort();
  }

  vsnprintf(g_current_test->fail_message, fail_msg_size + 1, msg_format, args2);
  va_end(args2);

  fputs(g_current_test->fail_message, stderr);
}

int main(int argc, const char *argv[]) {
  printf("Running tests...\n");

  struct test_suite *const testsuite_list = g_last_testsuite;

  // TODO: filtering
  size_t num_total_cases_run = 0;
  size_t num_total_cases_skipped = 0;
  size_t num_total_cases_failed = 0;

  struct test_suite *ts;
  for (ts = testsuite_list; ts != NULL; ts = ts->next_suite) {
    printf("\n-- Suite %s from %s running...\n", ts->suite_id, ts->file);

    g_current_suite = ts;

    if (ts->setup_hook)
      ts->setup_hook(ts);

    struct test_case *tc;
    for (tc = ts->cases; tc != NULL; tc = tc->next_case) {
      printf("\n--- Test %s running...\n", tc->case_id);

      g_current_test = tc;

      struct test_run_context ctx = {
        .tsuite = ts,
        .tcase = tc
      };

      if (ts->case_setup_hook)
        ts->case_setup_hook(&ctx);

      ts->num_cases_run++;
      num_total_cases_run++;

      // will be set to TEST_CASE_FAILED by _ctestfx_expect_fail() if the test fails
      tc->status = TEST_CASE_PASSED;
      tc->func(&ctx);

      if (tc->status == TEST_CASE_FAILED) {
        ts->num_cases_failed++;
        num_total_cases_failed++;
      }

      if (ts->case_teardown_hook)
        ts->case_teardown_hook(&ctx);

      printf("\n--- Test %s %s\n", tc->case_id, tc->status == TEST_CASE_FAILED ? "FAILED" : "PASSED");
    }

    if (ts->teardown_hook)
      ts->teardown_hook(ts);

    printf("\n-- Suite %s from %s finished: %zu / %zu passed, %zu skipped\n", ts->suite_id, ts->file, ts->num_cases_run - ts->num_cases_failed, ts->num_cases_run, ts->num_cases_total - ts->num_cases_run);
  }

  g_current_suite = NULL;
  g_current_test = NULL;

  // TODO: only in verbose
  printf("\nSUMMARY:\n");

  for (ts = testsuite_list; ts != NULL; ts = ts->next_suite) {
    printf("\nSuite %s in %s: ", ts->suite_id, ts->file);

    if (ts->num_cases_run == 0) {
      printf("Skipped\n");
      continue;
    }

    printf("%zu / %zu (%zu skipped):\n", ts->num_cases_run - ts->num_cases_failed, ts->num_cases_run, ts->num_cases_total - ts->num_cases_run);

    struct test_case *tc;
    for (tc = ts->cases; tc != NULL; tc = tc->next_case) {
      printf(" - Test %s: ", tc->case_id);

      switch (tc->status) {
      case TEST_CASE_SKIPPED: printf("Skipped\n"); break;
      case TEST_CASE_PASSED: printf("Pass\n"); break;
      case TEST_CASE_FAILED:
        printf("FAIL at %s:%d in %s: %s!\n", ts->file, tc->fail_line, tc->fail_func, tc->fail_message);
        break;
      }
    }
  }

  printf("\nTotal %zu / %zu passed, %zu skipped\n", num_total_cases_run - num_total_cases_failed, num_total_cases_run, num_total_cases_skipped);

  return num_total_cases_failed > 0 ? 1 : 0;
}
