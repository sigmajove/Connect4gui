#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <utility>
#include <vector>

class Board {
 public:
  // A Board is a 6x7 matrix of values.
  // A row is in the range [0:6] and a col is in the range [0:5].
  // A value is either:
  //    0 (empty),
  //    1 (occupied by Player 1),
  // or 2 (occupied by Player 2).

  static constexpr std::size_t kNumRows = 6;
  static constexpr std::size_t kNumCols = 7;

  Board() { clear(); }

  Board(const Board&) = default;
  Board& operator=(const Board&) = default;

  // Just compares the values of the squares.
  friend bool operator==(const Board& lhs, const Board& rhs) {
    return lhs.data_ == rhs.data_;
  }

  // Set and Get functions.
  void set_value(std::size_t row, std::size_t col, uint8_t value);
  std::uint8_t get_value(std::size_t row, std::size_t col) const;

  // Returns the columns where drop and push are valid.
  std::vector<std::size_t> legal_moves() const;

  void drop(std::size_t column);

  void push(std::size_t column);
  void pop();

  void clear() {
    std::fill(data_.begin(), data_.end(), 0);
    stack_.clear();
    whose_turn_ = 1;
  }

  // Estimate the desirability of the board from the point of view
  // of the named player.
  // Score 1000 points if the player has four in a row.
  // Score -1000 points if the opponent has four in a row.
  // For each combo with only player's tokens, score 1, 2, or 3
  // points depending on the number of tokens.
  // For each combo with only the opponent's tokens, score -1, -2, or -3,
  // depending on the number of tokens.
  int heuristic(std::uint8_t player) const;

  using Coord = std::pair<std::size_t, std::size_t>;

  // Calls visit with each of the four-in-a-row coordinate possibilities.
  static void combos(
      std::function<void(Coord a, Coord b, Coord c, Coord d)> visit);

 private:
  static constexpr std::size_t kNumValues = kNumRows * kNumCols;
  static constexpr std::size_t kBitsPerValue = 2;
  static constexpr std::size_t kValuesPerByte = 8 / kBitsPerValue;
  static constexpr std::size_t kNumBytes =
      (kNumValues + kValuesPerByte - 1) / kValuesPerByte;

  std::array<uint8_t, kNumBytes> data_;
  std::vector<Coord> stack_;
  uint8_t whose_turn_ = 1;
};
