int main_dump_pgroot_region(int argc, const char **argv) {
  dune_register_pgflt_handler(&pgflt_handler);

  LOG("pgroot (cr3/pml4) = %p\n", pgroot);

  vaddr_t base = (vaddr_t)pgroot - PGSIZE;
  size_t i;
  for (i = 1; i < 35; i++) { // 35 max
    LOG("--- Page %zu:\n", i);
    dump_pt(stderr, (ptent_t *)(base + i * PGSIZE), PT_L4);
  }

  pgallocwrite();

  LOG(" ------------------- AGAIN ----------------\n");

  for (i = 1; i < 35; i++) { // 35 max
    LOG("--- Page %zu:\n", i);
    dump_pt(stderr, (ptent_t *)(base + i * PGSIZE), PT_L4);
  }

  dune_procmap_dump();
  return 0;
}

int main_ptmapping(int argc, const char **argv) {
  main_dump_pml4(argc, argv);

  uint64_t *p = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  ASSERT(p != MAP_FAILED, "mmap error\n");

  LOG("p = %p\n", p);
  *p = 0xDEADBEEF;
  LOG("Wrote value: 0x%lx\n", *p);

  paddr_t ppa = dune_va_to_pa(p);
  LOG("pa = 0x%lx\n", ppa);

  pte_t *ppte;
  enum pt_level level = pt_walk((void *)ppa, PT_L1, OUT &ppte);
  LOG("Got level %u PPTE: %p\n", level, ppte);
  LOG("PTE = 0x%lx\n", *ppte);

  return 0;
}

int main2(int argc, const char **argv) {
  int result;
  uint64_t *p = mmap(NULL, 512, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  ASSERT(p != MAP_FAILED, "mmap error\n");

  LOG("mmap addr: %p\n", p);

  uintptr_t pa = dune_mmap_addr_to_pa(p);
  LOG("dune_mmap_addr_to_pa: %lx\n", pa);

  // int dune_vm_lookup(ptent_t *root, void *va, int create, ptent_t **pte_out)
  ptent_t *pte = NULL;
  if ((result = dune_vm_lookup(pgroot, p, 0, OUT &pte)) < 0) {
    LOG("dune_vm_lookup failed (pte at %p)!\n", pte);
  } else {
    LOG("dune_vm_lookup => pte at %p, value: 0x%lx\n", pte, *pte);
  }

  *p = 0xBEEFBABEuL;
  LOG("Wrote value: %lx\n", *p);

  pte = NULL;
  /*if ((result = dune_vm_lookup(pgroot, p, 0, OUT &pte)) < 0) {
    LOG("dune_vm_lookup2 failed (pte at %p)!\n", pte);
  } else {
    LOG("dune_vm_lookup2 => pte at %p, value: 0x%lx\n", pte, *pte);
  }*/

  /*void *remap = mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
  ASSERT(remap, "mmap error on remap\n");

  uintptr_t pa2 = dune_mmap_addr_to_pa(remap);
  LOG("mmap2: va = %p, pa = %lx\n", remap, pa2);*/

  /*for (int i = 0; i < 100; i++) {
    void *ptr = dune_mmap(NULL, PGSIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if (ptr == MAP_FAILED) {
      LOG("mmap %d failed!\n");
      break;
    }

    LOG("mmap %d => %p\n", i, ptr);
    LOG("\tPA = 0x%lx\n", dune_mmap_addr_to_pa(ptr));
  }*/

  /*result = dune_vm_map_phys(pgroot, remap, PGSIZE, (void *)pa, PERM_R | PERM_W);
  ASSERT(result == 0, "dune_vm_map_phys error!\n");

  uint64_t *remapped_p = (uint8_t *)remap + get_page_offset(p);
  LOG("Reading remapped value...\n");
  LOG("Read remapped value: %lx\n", *remapped_p);*/

  return 0;
}

static int idmap(void *base, paddr_t start, size_t len) {
  // I think this is necessary to update the EPT
  void *result = mmap(base, len, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
  if (result == MAP_FAILED) {
    LOG("failed for %p!\n", base);
    return 1;
  }

  if (result != base) {
    LOG("didn't get expected: base = %p, result = %p\n", base, result);
    return 2;
  }

  // map the requested physical memory to it
  //return dune_vm_map_phys(pgroot, base, len, (void *)start, PERM_R | PERM_W);
  return pt_map_region(start, (vaddr_t)base, len, PTE_W);
}