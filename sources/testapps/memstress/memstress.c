#include <stdlib.h>
#include <stdio.h>

enum {
  DEFAULT_NUM_ALLOCS = 10000,
  DEFAULT_ALLOC_SIZE = 32,
  DEFAULT_REPORT_INTERVAL = 1000,
  DEFAULT_NUM_ITERATIONS = 3
};

#define LOG(...) fprintf(stderr, __VA_ARGS__)

int main(int argc, const char *argv[]) {
  long long num_allocs = (argc < 2 ? DEFAULT_NUM_ALLOCS : atoll(argv[1]));
  LOG("Number of allocations: %lld\n", num_allocs);

  long size_allocs = (argc < 3 ? DEFAULT_ALLOC_SIZE : atol(argv[2]));
  LOG("Allocation size: %ld\n", size_allocs);

  if (size_allocs < sizeof(void *)) {
    LOG("Error: has to be minimum %zu bytes!\n", sizeof(void *));
    return -1;
  }

  long long report_interval = (argc < 4 ? DEFAULT_REPORT_INTERVAL : atoll(argv[3]));
  LOG("Reporting interval: %lld\n", report_interval);

  long num_iters = (argc < 5 ? DEFAULT_NUM_ITERATIONS : atol(argv[4]));
  LOG("Number of malloc/free iterations: %ld", num_iters);

  for (long iter = 0; iter < num_iters; iter++) {
    LOG("\n\nStarting iteration %ld...\n", iter);

    void *head = NULL;
    void **p = &head;
    for (long long i = 0; i < num_allocs; i++) {
      void **newp = (void **)calloc(1, size_allocs);
      if (!newp) {
        LOG("Allocation %lld failed!\n", i);
        return -1;
      }

      *p = (void *)newp;
      p = newp;

      if (i % report_interval == 0)
        LOG("%lld... ", i);
    }

    *p = NULL;

    LOG("\n\nDone allocating, going to free it all now...\n");

    void **curr = (void **)*(void **)head;
    long long curr_i = 0;
    while (curr) {
      void **next = (void **)*curr;
      free(curr);

      if (curr_i % report_interval == 0)
        LOG("%lld... ", curr_i);

      curr = next;
      curr_i++;
    }

    if (curr_i + 1 != num_allocs) {
      LOG("\nERROR: number of free()-s (%lld) differ from the number of allocations (%lld)!\n",  curr_i, num_allocs);
      abort();
    }
  }

  LOG("\n\nAll done!\n");
  return 0;
}
