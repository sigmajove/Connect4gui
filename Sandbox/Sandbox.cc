#include <iostream>

#include "..\board.h"

int main() {
  Board b;
  const auto [value, path] =
      b.alpha_beta_trace(/*depth=*/10,
                         /*alpha=*/std::numeric_limits<int>::min(),
                         /*beta=*/std::numeric_limits<int>::max(),
                         /*maximizing=*/true);

  for (std::size_t col : path) {
    std::cout << col << "\n";
    b.drop(col);
    std::cout << b.image() << '\n';
  }
}
