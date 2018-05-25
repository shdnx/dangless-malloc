#include <stdio.h>
#include <stdlib.h>

#include "testfx/testfx.h"

struct test_suite *g_current_suite = NULL;
struct test_case *g_current_test = NULL;
struct test_case_result *g_current_result = NULL;

static size_t g_num_testsuites = 0;
static size_t g_num_testcases_total = 0;
static size_t g_num_testcases_failed = 0;

static struct test_suite *g_testsuites = NULL;

#define LOG(...) fprintf(stderr, __VA_ARGS__)

void testsuite_register(struct test_suite *suite) {
  if (!g_testsuites) {
    g_testsuites = suite;
  } else {
    suite->next_suite = g_testsuites;
    g_testsuites = suite;
  }

  LOG("Registered test suite: %s\n", suite->name);
  g_num_testsuites++;
}

void _test_assert_fail(struct test_case *tc, struct test_case_result *tcr, int line, char *message) {
  tcr->pass = false;
  tcr->message = message;
  tcr->line = line;
}

static void testsuite_run(struct test_suite *tsuite) {
  LOG("Running suite: \"%s\"\n", tsuite->name);

  tsuite->func(tsuite);
}

struct test_case_result *testcase_result_alloc(void) {
  struct test_case_result *result = malloc(sizeof(struct test_case_result));
  result->case_name = NULL;
  result->pass = true;
  result->message = NULL;
  result->line = 0;
  result->next_result = NULL;
  return result;
}

bool testcase_prepare_run(struct test_suite *tsuite, struct test_case *tcase, /*OUT*/ struct test_case_result **presult) {
  LOG("Running testcase: \"%s\"\n", tcase->name);

  tsuite->num_cases++;

  // TODO: check if test case should be run (filtering) - return false if it shouldn't

  g_num_testcases_total++;
  tsuite->num_case_results++;

  //LOG("\t%s... ", tcase->name);

  // prepare the result
  struct test_case_result *result = testcase_result_alloc();
  result->case_name = tcase->name;

  /*OUT*/ *presult = result;

  g_current_suite = tsuite;
  g_current_test = tcase;
  g_current_result = result;
  return true;
}

static void testsuite_register_result(struct test_suite *tsuite, struct test_case_result *result) {
  if (tsuite->case_results) {
    result->next_result = tsuite->case_results;
    tsuite->case_results = result;
  } else {
    tsuite->case_results = result;
  }
}

void testcase_register_run(struct test_suite *tsuite, struct test_case *tcase, struct test_case_result *result) {
  // record the result
  testsuite_register_result(tsuite, result);

  // summary
  if (result->pass) {
    //LOG("pass\n");
    LOG("Test case %s passed!\n", tcase->name);
  } else {
    g_num_testcases_failed++;
    tsuite->num_failed_cases++;

    //LOG("FAIL\n");
    LOG("Test case %s failed!\n", tcase->name);
  }

  g_current_suite = NULL;
  g_current_test = NULL;
  g_current_result = NULL;
}

int main(int argc, const char **argv) {
  // TODO: testsuite/testcase filters, e.g. by filename prefix?
  struct test_suite *ts;
  for (ts = g_testsuites; ts != NULL; ts = ts->next_suite) {
    testsuite_run(ts);
  }

  if (g_num_testcases_failed > 0) {
    // TODO: only in verbose
    fprintf(stderr, "\nFailure details:\n");

    for (ts = g_testsuites; ts != NULL; ts = ts->next_suite) {
      if (ts->num_failed_cases == 0)
        continue;

      fprintf(stderr, "\nTest suite \"%s\" (%zu failures out of %zu):\n", ts->name, ts->num_failed_cases, ts->num_case_results);

      struct test_case_result *tcr;
      for (tcr = ts->case_results;
           tcr != NULL;
           tcr = tcr->next_result) {
        if (tcr->pass)
          continue;

        fprintf(stderr, " - In \"%s\" at %s:%d: %s\n", tcr->case_name, ts->file, tcr->line, tcr->message);
      }
    }
  }

  fprintf(stderr, "Finished! Passed: %zu / %zu\n", g_num_testcases_total - g_num_testcases_failed, g_num_testcases_total);

  return g_num_testcases_failed > 0 ? 1 : 0;
}
