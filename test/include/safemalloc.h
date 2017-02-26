#ifndef SAFEMALLOC_H
#define SAFEMALLOC_H

#include "common.h"

void *safe_malloc(size_t sz);
void safe_free(void *p);

#endif