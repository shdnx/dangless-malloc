#ifndef TESTFX_TESTSUITE_H
#define TESTFX_TESTSUITE_H

// Setup and teardown hooks: these hooks run before and after each test case respectively.
#define TEST_SETUP() void testcase_setup_hook(void)
#define TEST_TEARDOWN() void testcase_teardown_hook(void)

#include "testfx/internals/testsuite.h"

// Defines a test suite, a group of test cases.
#define TEST_SUITE(NAME) \
  _TEST_SUITE_IMPL(NAME, _TEST_SUITE_FNAME_GEN())

// Defines a test case.
#define TEST(NAME) \
  _TEST_CASE_IMPL(NAME, _TEST_CASE_FNAME_GEN())

#endif
