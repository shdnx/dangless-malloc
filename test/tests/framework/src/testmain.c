#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "testfx.h"

static size_t g_num_testsuites = 0;
static size_t g_num_testcases_total = 0;
static size_t g_num_testcases_failed = 0;

static struct test_suite *g_testsuites = NULL;

void testsuite_register(struct test_suite *suite) {
  if (!g_testsuites) {
    g_testsuites = suite;
  } else {
    suite->next_suite = g_testsuites;
    g_testsuites = suite;
  }

  g_num_testsuites++;
}

void _assert_fail(struct test_case *tc, struct test_case_result *tcr, int line, char *message) {
  tcr->pass = false;
  tcr->message = message;
  tcr->line = line;
}

static void testsuite_run(struct test_suite *tsuite) {
  fprintf(stderr, "Running suite: \"%s\"\n", tsuite->name);

  tsuite->func(tsuite);
}

void testcase_run(struct test_suite *tsuite, struct test_case *tcase) {
  tsuite->num_cases++;

  // TODO: check if test case should be run (filtering)

  g_num_testcases_total++;
  tsuite->num_case_results++;

  fprintf(stderr, "\t%s... ", tcase->name);

  // prepare the result
  struct test_case_result *result = malloc(sizeof(struct test_case_result));
  result->case_name = tcase->name;
  result->pass = true;
  result->message = NULL;
  result->line = 0;
  result->next_result = NULL;

  // run the test case
  tcase->func(tsuite, tcase, result);

  // record the result
  if (tsuite->case_results) {
    result->next_result = tsuite->case_results;
    tsuite->case_results = result;
  } else {
    tsuite->case_results = result;
  }

  // summary
  if (result->pass) {
    fprintf(stderr, "pass\n");
  } else {
    g_num_testcases_failed++;
    tsuite->num_failed_cases++;

    fprintf(stderr, "FAIL\n");
  }
}

int main(int argc, const char **argv) {
  // TODO: testsuite/testcase filters, e.g. by filename prefix?
  struct test_suite *ts;
  for (ts = g_testsuites; ts != NULL; ts = ts->next_suite) {
    testsuite_run(ts);
  }

  // TODO: verbose
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

  fprintf(stderr, "Finished! Passed: %zu / %zu\n", g_num_testcases_total - g_num_testcases_failed, g_num_testcases_total);

  return 0;
}
