#include <iostream>

#include "..\board.h"

int main() {
  Board b;
  try {
    const auto [eval, path] = b.BruteForce(1e26, 1);

    std::cout << "Result " << static_cast<int>(eval) << "\n";
    std::cout << "Path size " << path.size();
    for (std::size_t col : path) {
      std::cout << col << "\n";
    }
  } catch (const std::exception& e) {
    std::cout << "Error " << e.what() << "\n";
  } catch (...) {
    std::cout << "Unknown exception\n";
  }
}
