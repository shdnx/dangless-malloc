#include <stdio.h>

#include "dangless/common.h"
#include "dangless/virtmem.h"
#include "dangless/platform/mem.h"

#define PRINTF_INDENT(INDENT_LENGTH, FORMAT, ...) \
  printf("%*s" FORMAT, (INDENT_LENGTH), "", __VA_ARGS__)

void dump_mappings(FILE *os, vaddr_t va_start, vaddr_t va_end) {
  vaddr_t va;
  paddr_t pa;
  enum pt_level level;

  bool mapped = false;
  bool identity = false;
  vaddr_t start = 0;

  for (va = va_start; va < va_end; va += (1uL << pt_level_shift(level))) {
    pa = pt_resolve_page((void *)va, OUT &level);

    if (pa) {
      // it's mapped
      if (!mapped && start != 0) {
        fprintf(os, "Unmapped region 0x%lx - 0x%lx\n", start, va);
        start = 0;
        identity = false;
      }

      mapped = true;
      if (!start)
        start = va;

      if (pa == va) {
        // it's identity-mapped
        identity = true;
      } else {
        // it's non-identity mapped
        if (identity && start != 0) {
          fprintf(os, "Identity-mapped region 0x%lx - 0x%lx\n", start, va);
          identity = false;
        }

        fprintf(os, "Non-identity mapping (level %u): VA 0x%lx => PA 0x%lx\n", level, va, pa);
      }
    } else {
      // it's unmapped
      if (mapped && identity && start != 0) {
        fprintf(os, "Identity-mapped region 0x%lx - 0x%lx\n", start, va);
        start = 0;
      }

      mapped = false;
      identity = false;
      if (!start)
        start = va;
    }
  }

  if (start != 0) {
    if (mapped && identity) {
      fprintf(os, "Final identity-mapped region 0x%lx - 0x%lx\n", start, va_end);
    } else if (!mapped) {
      fprintf(os, "Final unmapped region 0x%lx - 0x%lx\n", start, va_end);
    }
  }
}

void dump_pte(FILE *os, pte_t pte, enum pt_level level) {
  fprintf(os, "PTE addr = 0x%lx", pte & PTE_FRAME);

#define HANDLE_BIT(BIT) if (pte & (BIT)) fprintf(os, " " #BIT)

  HANDLE_BIT(PTE_V);
  HANDLE_BIT(PTE_W);
  HANDLE_BIT(PTE_U);
  HANDLE_BIT(PTE_NX);

  if (level == PT_L1) {
    HANDLE_BIT(PTE_PAT);
  } else {
    HANDLE_BIT(PTE_PS);
  }

#undef HANDLE_BIT

  fprintf(os, " (raw: 0x%lx)\n", pte);
}

void dump_pt(FILE *os, pte_t *pt, enum pt_level level) {
  unsigned level_shift = pt_level_shift(level);

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    pte_t pte = pt[i];
    if (pte) {
      fprintf(os, "Mapped: 0x%lx - 0x%lx by ", (uintptr_t)i << level_shift, (uintptr_t)(i + 1) << level_shift);
      dump_pte(os, pte, level);
    }
  }
}

void dump_pt_summary(FILE *os, pte_t *pt, enum pt_level level) {
  unsigned level_shift = pt_level_shift(level);

  bool mapped = false;
  size_t count = 0;

  size_t i;
  for (i = 0; i < PT_NUM_ENTRIES; i++) {
    pte_t pte = pt[i];
    if (pte) {
      if (!mapped && count > 0) {
        fprintf(os, "Unmapped: 0x%lx - 0x%lx [%zu entries]\n",
          (uintptr_t)(i - count) << level_shift,
          (uintptr_t)i << level_shift,
          count);

        count = 0;
      }

      mapped = true;
      count++;
    } else {
      if (mapped && count > 0) {
        fprintf(os, "Mapped: 0x%lx - 0x%lx [%zu entries]\n",
          (uintptr_t)(i - count) << level_shift,
          (uintptr_t)i << level_shift,
          count);

        count = 0;
      }

      mapped = false;
      count++;
    }
  }

  if (count > 0) {
    fprintf(os, "%s: 0x%lx - 0x%lx [%zu entries]\n",
      mapped ? "Mapped" : "Unmapped",
      (uintptr_t)(i - count) << level_shift,
      (uintptr_t)i << level_shift,
      count);
  }
}