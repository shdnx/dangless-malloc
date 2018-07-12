#include <iostream>
#include <fstream>
#include <cstdlib>
#include <ctime>

constexpr const size_t TARGET_SIZE = 8250; // 8192
constexpr const size_t REPORT_INTERVAL = 100;

int main() {
  std::srand(std::time(nullptr));

  std::cerr << "\n---------------------------------------\n";
  std::cerr << "ALLOC BEGIN\n";
  std::ofstream os("output.txt");
  std::cerr << "ALLOC END\n";

  int data;

  std::cerr << "STARTING to write to output.txt...\n";

  for (size_t i = 0; i < TARGET_SIZE / sizeof(data); i++) {
    data = std::rand();
    os << data;

    if (i % REPORT_INTERVAL == 0)
      std::cerr << i << "... ";

    if (!os.good()) {
      std::cerr << "\nSTREAM FAIL at i = " << i << " (size " << i * sizeof(data) << ")!\n";
      std::cerr << "\n---------------------------------------\n";
      return -1;
    }
  }

  if (!os.good()) {
    std::cerr << "\nSTREAM FAIL at end!\n";
    std::cerr << "\n---------------------------------------\n";
    return -2;
  }

  std::cerr << "SUCESSS\n";
  std::cerr << "\n---------------------------------------\n";
  return 0;
}
