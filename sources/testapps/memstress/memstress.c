#include <stdlib.h>
#include <stdio.h>

enum {
  DEFAULT_NUM_ALLOCS = 10000,
  DEFAULT_ALLOC_SIZE = 32,
  DEFAULT_REPORT_INTERVAL = 1000
};

#define LOG(...) fprintf(stderr, __VA_ARGS__)

int main(int argc, const char *argv[]) {
  long long num_allocs = (argc < 2 ? DEFAULT_NUM_ALLOCS : atoll(argv[1]));
  LOG("Number of allocations: %lld\n", num_allocs);

  long size_allocs = (argc < 3 ? DEFAULT_ALLOC_SIZE : atol(argv[2]));
  LOG("Allocation size: %ld\n", size_allocs);

  long long report_interval = (argc < 4 ? DEFAULT_REPORT_INTERVAL : atoll(argv[3]));
  LOG("Reporting interval: %lld\n", report_interval);

  LOG("Starting...\n");

  void *head = NULL;
  void **p = &head;
  for (long long i = 0; i < num_allocs; i++) {
    void **newp = (void **)malloc(size_allocs);
    if (!newp) {
      LOG("Allocation %lld failed!\n", i);
      return -1;
    }

    *p = (void *)newp;
    p = newp;

    if (i % report_interval == 0)
      LOG("%lld... ", i);
  }

  LOG("\nAll done!\n");
  LOG("Deallocations are left as an exercise to the kernel, lol\n");
  return 0;
}
