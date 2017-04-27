#pragma once

#include "common.h"
#include "virtmem.h"

void dump_mappings(vaddr_t va_start, vaddr_t va_end);

void dump_pte(FILE *os, pte_t pte, enum pt_level level);
void dump_pt(pte_t *pt, enum pt_level level);
void dump_pt_summary(pte_t *pt, enum pt_level level);