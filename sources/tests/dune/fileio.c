#include <stdio.h>

#include "dunetest.h"

TEST_SUITE(fileio);

TEST_SUITE_SETUP() {
  dunetest_init();
}

TEST_CASE(file_read) {
  FILE *fp = fopen("data.txt", "r");
  EXPECT_NOT_NULL(fp);

  char *buffer = TMALLOC(char[32]);
  size_t nchars = fread(buffer, sizeof(char), 31, fp);
  buffer[nchars] = '\0';

  EXPECT_EQUALS_STR(buffer, "potato foo bar and baz, friends");

  fclose(fp);

  TFREE(buffer);
}
