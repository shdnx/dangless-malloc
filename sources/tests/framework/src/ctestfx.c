#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "ctestfx/ctestfx.h"

#undef LOG
#undef LOG_VERBOSE
#undef LOGV

static struct test_suite *g_last_testsuite;
static size_t g_num_testsuites_total;

static struct test_suite *g_current_suite;
static struct test_case *g_current_test;

struct arg_options {
  bool default_enabled;
  bool verbose;
  bool debug;
};

struct arg_options g_options = {
  .default_enabled = true,
  .verbose = false,
  .debug = false
};

#define LOG_REGISTER(...) \
   printf(__VA_ARGS__)

#define LOG_DEBUG(...) \
  if (g_options.debug) fprintf(stderr, "[debug] " __VA_ARGS__)

#define LOG_VERBOSE(...) \
  if (g_options.verbose) printf(__VA_ARGS__)

void _ctestfx_log(enum log_mode lmode, const char *file, int line, const char *format, ...) {
  if (lmode == LOG_VERBOSE && !g_options.verbose)
    return;

  fprintf(stderr, "[%s:%d] ", file, line);

  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
}

static void last_testsuite_register_end(void) {
  if (g_last_testsuite) {
    LOG_REGISTER("-- Done registering test suite %s: %zu test cases\n\n", g_last_testsuite->suite_id, g_last_testsuite->num_cases_total);
  }
}

void _testsuite_register(struct test_suite *tsuite) {
  last_testsuite_register_end();

  tsuite->next_suite = g_last_testsuite;
  g_last_testsuite = tsuite;

  g_num_testsuites_total++;

  LOG_REGISTER("-- Registering test suite '%s' from %s\n", tsuite->suite_id, tsuite->file);
}

void _testsuite_register_setup(test_suite_hook_t func) {
  g_last_testsuite->setup_hook = func;

  LOG_REGISTER("---- Registered setup hook %p\n", func);
}

void _testsuite_register_teardown(test_suite_hook_t func) {
  g_last_testsuite->teardown_hook = func;

  LOG_REGISTER("---- Registered teardown hook %p\n", func);
}

void _testsuite_register_case_setup(test_case_hook_t func) {
  g_last_testsuite->case_setup_hook = func;

  LOG_REGISTER("---- Registered case setup hook %p\n", func);
}

void _testsuite_register_case_teardown(test_case_hook_t func) {
  g_last_testsuite->case_teardown_hook = func;

  LOG_REGISTER("---- Registered case teardown hook %p\n", func);
}

void _testcase_register(struct test_case *tcase) {
  tcase->next_case = g_last_testsuite->cases;
  g_last_testsuite->cases = tcase;

  g_last_testsuite->num_cases_total++;

  LOG_REGISTER("---- Registered test case '%s'\n", tcase->case_id);
}

__attribute__((noreturn))
static void fail_out_of_memory(void) {
  fprintf(stderr, "!!! Failed to record test failure: out of memory!\n");
  perror("Allocation error: ");
  abort();
}

char *_ctestfx_preformat_fail_message(const char *format, ...) {
  static char *s_tmp_buffer;
  static size_t s_tmp_buffer_size = 512;

  if (!s_tmp_buffer) {
    s_tmp_buffer = malloc(s_tmp_buffer_size);

    if (!s_tmp_buffer)
      fail_out_of_memory();
  }

  // we need 2 copies of the argument list, because the first vsnprintf() call that is needed to compute the buffer size will consume args1
  va_list args, args2;
  va_start(args, format);
  va_copy(args2, args);

  int preformat_msg_size = vsnprintf(s_tmp_buffer, s_tmp_buffer_size, format, args);
  if (preformat_msg_size > s_tmp_buffer_size) {
    // the string writing got truncated, we need to extend it
    s_tmp_buffer_size = preformat_msg_size + 1; // don't forget the null terminator!
    s_tmp_buffer = realloc(s_tmp_buffer, s_tmp_buffer_size);
    if (!s_tmp_buffer)
      fail_out_of_memory();

    preformat_msg_size = vsnprintf(s_tmp_buffer, s_tmp_buffer_size, format, args2);
  }

  va_end(args2);
  va_end(args);

  return s_tmp_buffer;
}

// As msg_format, this takes a string pre-formatted by _ctestfx_preformat_fail_message().
// We have to do formatting in two steps, otherwise we run into an annoying GCC bug with version 5.4.0 whereby it fucks up va_list forwarding (see vafwd-gcc-bug.c).
static char *create_fail_message(const char *msg_format, va_list args) {
  va_list args2;
  va_copy(args2, args);

  const int msg_size = vsnprintf(NULL, 0, msg_format, args);

  char *msg = malloc(msg_size + 1);
  if (!msg)
    fail_out_of_memory();

  vsnprintf(msg, msg_size + 1, msg_format, args2);
  va_end(args2);

  return msg;
}

void _ctestfx_expect_fail(const char *func, int line, const char *msg_format, ...) {
  g_current_test->status = TEST_CASE_FAILED;

  g_current_test->fail_func = func;
  g_current_test->fail_line = line;

  va_list args;
  va_start(args, msg_format);
  g_current_test->fail_message = create_fail_message(msg_format, args);
  va_end(args);

  fputs("!! EXPECTATION FAILED: ", stderr);
  fputs(g_current_test->fail_message, stderr);
}

static struct test_suite *find_suite(const char *id) {
  struct test_suite *ts;
  for (ts = g_last_testsuite; ts; ts = ts->next_suite) {
    if (strcmp(id, ts->suite_id) == 0)
      return ts;
  }

  return NULL;
}

static struct test_case *find_case(const struct test_suite *ts, const char *id) {
  struct test_case *tc;
  for (tc = ts->cases; tc; tc = tc->next_case) {
    if (strcmp(id, tc->case_id) == 0)
      return tc;
  }

  return NULL;
}

static int handle_args(int argc, const char *argv[], /*REF*/ struct arg_options *options) {
  bool default_enabled = true;

  bool set_specified_enabled = true;
  const char **argp;
  for (argp = &argv[1]; *argp; argp++) {
    const char *arg = *argp;

    if (strncmp(arg, "-", 1) == 0) {

#define HANDLE_FLAG(SHORT, LONG) \
  else if (strcmp(arg + 1, #SHORT) == 0 \
      || strcmp(arg + 1, "-" #LONG) == 0)

#define HANDLE_LONG_FLAG(LONG) \
  else if (strcmp(arg + 1, "-" #LONG) == 0)

      if (false) {}

      HANDLE_FLAG(o, only) {
        default_enabled = false;
        set_specified_enabled = true;
      }

      HANDLE_FLAG(i, include) {
        set_specified_enabled = true;
      }

      HANDLE_FLAG(x, exclude) {
        set_specified_enabled = false;
      }

      HANDLE_FLAG(v, verbose) {
        options->verbose = true;
      }

      HANDLE_LONG_FLAG(debug) {
        options->debug = true;
      }

      else {
        fprintf(stderr, "Warning: ignoring unrecognized flag '%s'\n", arg);
      }
    } else {
      char *separator = strstr(arg, "/");
      /*owned*/ char *suite_id;
      const char *case_id;
      if (separator) {
        const size_t suite_id_len = separator - arg;

        suite_id = malloc(suite_id_len + 1);
        strncpy(suite_id, arg, suite_id_len);
        suite_id[suite_id_len] = '\0';

        case_id = arg + suite_id_len + 1;
      } else {
        suite_id = strdup(arg);
        case_id = NULL;
      }

      struct test_suite *ts = find_suite(suite_id);
      if (!ts) {
        fprintf(stderr, "Error: cannot find test suite '%s'!\n", suite_id);
        return 1;
      }

      if (case_id) {
        struct test_case *tc = find_case(ts, case_id);
        if (!tc) {
          fprintf(stderr, "Error: cannot find test case '%s' in test suite '%s'!\n", case_id, suite_id);
          return 1;
        }

        tc->enabled = set_specified_enabled ? TEST_ENABLED : TEST_DISABLED;
      } else {
        ts->enabled = set_specified_enabled ? TEST_ENABLED : TEST_DISABLED;
      }

      free(suite_id);
    }
  }

  options->default_enabled = default_enabled;
  return 0;
}

static bool suite_has_enabled_cases(const struct test_suite *ts) {
  struct test_case *tc;
  for (tc = ts->cases; tc; tc = tc->next_case) {
    if (tc->enabled == TEST_ENABLED
        || (
          tc->enabled == TEST_ENABLED_DEFAULT
          && g_options.default_enabled
        ))
      return true;
  }

  return false;
}

static bool is_suite_enabled(const struct test_suite *ts) {
  return ts->enabled == TEST_ENABLED
    || (
      ts->enabled == TEST_ENABLED_DEFAULT
      && suite_has_enabled_cases(ts)
    );
}

static bool is_case_enabled(const struct test_case *tc, const struct test_suite *ts) {
  if (tc->enabled == TEST_ENABLED)
    return true;

  if (tc->enabled == TEST_DISABLED)
    return false;

  return ts->enabled == TEST_ENABLED
    || g_options.default_enabled;
}

int main(int argc, const char *argv[]) {
  last_testsuite_register_end();

  int result;
  if ((result = handle_args(argc, argv, /*REF*/ &g_options)) != 0)
    return result;

  printf("Running tests...\n");

  struct test_suite *const testsuite_list = g_last_testsuite;

  size_t num_total_cases_run = 0;
  size_t num_total_cases_skipped = 0;
  size_t num_total_cases_failed = 0;

  struct test_suite *ts;
  for (ts = testsuite_list; ts; ts = ts->next_suite) {
    if (ts->num_cases_total == 0)
      continue;

    if (!is_suite_enabled(ts)) {
      num_total_cases_skipped += ts->num_cases_total;
      continue;
    }

    printf("\n-- Suite %s from %s running...\n", ts->suite_id, ts->file);

    g_current_suite = ts;

    if (ts->setup_hook)
      ts->setup_hook(ts);

    struct test_case *tc;
    for (tc = ts->cases; tc; tc = tc->next_case) {
      if (!is_case_enabled(tc, ts)) {
        num_total_cases_skipped++;
        continue;
      }

      printf("\n---- Test %s running...\n\n", tc->case_id);

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
      tc->func();

      if (tc->status == TEST_CASE_FAILED) {
        ts->num_cases_failed++;
        num_total_cases_failed++;
      }

      if (ts->case_teardown_hook)
        ts->case_teardown_hook(&ctx);

      printf("\n---- Test %s %s\n", tc->case_id, tc->status == TEST_CASE_FAILED ? "FAILED" : "PASSED");
    }

    if (ts->teardown_hook)
      ts->teardown_hook(ts);

    printf("\n-- Suite %s from %s finished: %zu / %zu passed, %zu skipped\n", ts->suite_id, ts->file, ts->num_cases_run - ts->num_cases_failed, ts->num_cases_run, ts->num_cases_total - ts->num_cases_run);
  }

  g_current_suite = NULL;
  g_current_test = NULL;

  printf("\n------------------------------------------\n");
  printf(  "---------------- SUMMARY -----------------\n");
  printf(  "------------------------------------------\n\n");

  for (ts = testsuite_list; ts != NULL; ts = ts->next_suite) {
    if (ts->num_cases_run > 0 || g_options.verbose) {
      printf("Suite %s in %s", ts->suite_id, ts->file);
    }

    if (ts->num_cases_run == 0) {
      if (g_options.verbose) {
        if (ts->num_cases_total == 0) {
          printf(": empty\n");
        } else {
          printf(": skipped (%zu tests)\n", ts->num_cases_total);
        }
      }

      continue;
    }

    printf(" (%zu / %zu passed, %zu skipped)", ts->num_cases_run - ts->num_cases_failed, ts->num_cases_run, ts->num_cases_total - ts->num_cases_run);

    if (ts->num_cases_failed == 0 && !g_options.verbose) {
      printf("\n");
      continue;
    }

    printf(":\n");

    struct test_case *tc;
    for (tc = ts->cases; tc != NULL; tc = tc->next_case) {
      if (g_options.verbose) {
        printf(" - Test %s: ", tc->case_id);

        switch (tc->status) {
        case TEST_CASE_SKIPPED: printf("skipped\n"); break;
        case TEST_CASE_PASSED: printf("PASS\n"); break;
        case TEST_CASE_FAILED:
          printf("FAIL at %s:%d in %s: %s\n", ts->file, tc->fail_line, tc->fail_func, tc->fail_message);
          break;
        }
      } else if (is_case_enabled(tc, ts) && tc->status == TEST_CASE_FAILED) {
        printf(" - Test %s: FAIL at %s:%d in %s: %s\n", tc->case_id, ts->file, tc->fail_line, tc->fail_func, tc->fail_message);
      }
    }
  }

  printf("\nTOTAL %zu / %zu passed, %zu skipped\n", num_total_cases_run - num_total_cases_failed, num_total_cases_run, num_total_cases_skipped);

  return num_total_cases_failed > 0 ? 1 : 0;
}
