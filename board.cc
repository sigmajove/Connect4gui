#include "board.h"

#include <format>
#include <iostream>
#include <utility>

void Board::drop(std::size_t column) {
  for (std::size_t row = 0; row < 6; ++row) {
    if (get_value(row, column) == 0) {
      set_value(row, column, whose_turn_);
      break;
    }
  }
  whose_turn_ = 3 - whose_turn_;
}

void Board::combos(
    std::function<void(Coord a, Coord b, Coord c, Coord d)> visit) {
  for (std::size_t row = 0; row < 6; ++row) {
    for (std::size_t col = 0; col <= 3; ++col) {
      visit(std::make_pair(row, col), std::make_pair(row, col + 1),
            std::make_pair(row, col + 2), std::make_pair(row, col + 3));
    }
  }
  for (std::size_t row = 0; row <= 2; ++row) {
    for (std::size_t col = 0; col < 7; ++col) {
      visit(std::make_pair(row, col), std::make_pair(row + 1, col),
            std::make_pair(row + 2, col), std::make_pair(row + 3, col));
    }
  }

  for (std::size_t row = 0; row <= 2; ++row) {
    for (std::size_t col = 0; col <= 3; col++) {
      visit(std::make_pair(row, col), std::make_pair(row + 1, col + 1),
            std::make_pair(row + 2, col + 2), std::make_pair(row + 3, col + 3));
    }
  }

  for (std::size_t row = 0; row <= 2; ++row) {
    for (std::size_t col = 3; col < 7; col++) {
      visit(std::make_pair(row, col), std::make_pair(row + 1, col - 1),
            std::make_pair(row + 2, col - 2), std::make_pair(row + 3, col - 3));
    }
  }
}

int Board::heuristic(std::uint8_t player) const {
  const std::uint8_t other = 3 - player;
  bool four_for_me = false;
  bool four_for_him = false;
  int score = 0;
  combos([player, other, &four_for_me, &four_for_him, &score, this](
             Coord a, Coord b, Coord c, Coord d) {
    const std::uint8_t ac = get_value(a.first, a.second);
    const std::uint8_t bc = get_value(b.first, b.second);
    const std::uint8_t cc = get_value(c.first, c.second);
    const std::uint8_t dc = get_value(d.first, d.second);

    const int mine =
        (ac == player) + (bc == player) + (cc == player) + (dc == player);
    const int theirs =
        (ac == other) + (bc == other) + (cc == other) + (dc == other);

    if (mine > 0 && theirs > 0) {
      return;
    }

    if (mine == 4) {
      four_for_me = true;
    }
    if (theirs == 4) {
      four_for_him = true;
    }
    if (mine > 0) {
      score += mine;
#if 0
      std::cout << "mine=" << mine << "\n";
      std::cout << std::format("{} {}, {} {}, {} {}, {} {}\n", a.first,
                               a.second, b.first, b.second, c.first, c.second,
                               d.first, d.second);
#endif
    }
    if (theirs > 0) {
#if 0
      std::cout << "theirs=" << theirs << "\n";
#endif
      score -= theirs;
    }
  });

  return four_for_me ? 1000 : four_for_him ? -1000 : score;
}
