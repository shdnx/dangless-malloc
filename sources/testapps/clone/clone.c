#define _GNU_SOURCE
#include <sys/wait.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LOG(...) fprintf(stderr, __VA_ARGS__)

#define DIE(MSG) \
  do { \
    perror(MSG); \
    exit(EXIT_FAILURE); \
  } while (0)

static int *g_magic;

static int childproc(void *arg) {
  LOG("Hello I'm child proc!\n");
  LOG("Arg is: %s\n", (char *)arg);
  LOG("Magic is: %d\n", *g_magic);
  return 0;
}

enum {
  CHILD_STACK_SIZE = 4 * 1024
};

int main(int argc, char *argv[]) {
  if (argc == 1) {
    LOG("Need an argument to be passed to childproc!\n");
    return 1;
  }

  g_magic = malloc(sizeof(int));
  if (!g_magic)
    DIE("malloc magic");

  *g_magic = 0x1337;
  LOG("magic at %p is set to: %d\n", g_magic, *g_magic);

  char *childStack = malloc(CHILD_STACK_SIZE);
  if (!childStack)
    DIE("malloc child stack");

  pid_t childPid = clone(childproc, childStack + CHILD_STACK_SIZE, SIGCHLD, argv[1]);
  if (childPid == -1)
    DIE("clone");

  LOG("clone() returned %ld\n", (long)childPid);

  int childStatus;
  if (waitpid(childPid, &childStatus, 0) == -1)
    DIE("waitpid");

  LOG("child has terminated with status %d\n", childStatus);
  return EXIT_SUCCESS;
}
