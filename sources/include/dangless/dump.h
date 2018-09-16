#ifndef DANGLESS_DUMP_H
#define DANGLESS_DUMP_H

#include <stdio.h> // FILE *

#include "dangless/common.h"
#include "dangless/virtmem.h"

void dump_mappings(FILE *os, vaddr_t va_start, vaddr_t va_end);

void dump_pte(FILE *os, pte_t pte, enum pt_level level);
void dump_pt(FILE *os, pte_t *pt, enum pt_level level);
void dump_pt_summary(FILE *os, pte_t *pt, enum pt_level level);

#endif
