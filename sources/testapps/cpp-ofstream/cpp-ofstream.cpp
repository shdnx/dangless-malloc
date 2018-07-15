#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <typeinfo>

static void dump_mem_hex(const char *name, const char *type, const void *ptr, size_t len) {
  std::cerr
    << "MEMORY DUMP of '" << name << "'\n"
    << " - Type: " << type << '\n'
    << " - Size: " << len << '\n'
    << " - Address: " << std::hex << ptr << '\n';

  const char *p = (const char *)ptr;
  const char *end = (const char *)ptr + len;

  for (; (uintptr_t)p < (uintptr_t)end; p += sizeof(uint64_t)) {
    std::cerr
      << (const void *)p
      << '\t';

    /*const uint8_t *cp = (const uint8_t *)p;
    for (size_t i = 0; i < sizeof(uint64_t) / sizeof(uint8_t); i++, cp++) {
      std::cerr << ' ' << std::hex << *cp;
    }

    std::cerr << '\t';*/

    std::cerr
      << *(unsigned long long *)p
      << '\n';
  }

  std::cerr << std::dec << "END\n\n";
}

template<typename T>
static void dump_mem_hex(const char *name, const T &obj) {
  dump_mem_hex(name, typeid(T).name(), &obj, sizeof(T));
}

#define DUMP_MEM(VARNAME) dump_mem_hex(#VARNAME, VARNAME)

template<typename TData>
static int test_stream(std::ostream &os, size_t target_size, size_t report_interval) {
  std::cerr << "\n-- STARTING...\n";

  for (size_t i = 0; i < target_size / sizeof(TData); i++) {
    TData data = static_cast<TData>(std::rand());
    //TData data = static_cast<TData>((1000 + std::rand()) % 10000);
    //TData data = static_cast<TData>(0xBADF00D);

    //os << data;
    os.write(reinterpret_cast<char *>(&data), sizeof(data));

    if (i % report_interval == 0)
      std::cerr << i << "... ";

    if (!os.good()) {
      std::cerr << "\nSTREAM FAIL at i = " << i << " (size " << i * sizeof(TData) << ")!\n";
      //std::cerr << "---------------------------------------\n";
      return -1;
    }
  }

  if (!os.good()) {
    std::cerr << "\nSTREAM FAIL at end!\n";
    return -2;
  }

  std::cerr << "\nSUCESSS\n";
  return 0;
}

int main() {
  std::cerr << "\n-- MAIN BEGIN\n";

  std::srand(std::time(nullptr));

  int result;

  {
    std::cerr << "\n-- ALLOC BEGIN\n";

    std::ofstream os("output.dat", std::ios_base::binary);
    //std::ostringstream os;

    DUMP_MEM(os);

    std::cerr << "\n-- ALLOC END\n";

    result = test_stream<int64_t>(
      os,
      /*target_size=*/27000, // 8250
      /*report_interval=*/100
    );

    std::cerr << "\n-- DEALLOC BEGIN\n";
  } // destroy os

  std::cerr << "\n-- DEALLOC END\n";

  std::cerr << "\n-- MAIN END\n";
  return 0;
}
