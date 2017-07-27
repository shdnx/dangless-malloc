#ifndef TESTFX_COMMON_H
#define TESTFX_COMMON_H

#include <stdint.h>
#include <stdbool.h>

#define EVAL(V) V
#define STRINGIFY(V) #V

#define __DO_CONCAT2(A, B) A##B
#define CONCAT2(A, B) __DO_CONCAT2(A, B)

#define __DO_CONCAT3(A, B, C) A##B##C
#define CONCAT3(A, B, C) __DO_CONCAT3(A, B, C)

#endif
