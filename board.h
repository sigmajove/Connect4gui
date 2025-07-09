#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <functional>
#include <utility>

class Board {
 public:
  // A Board is a 6x7 matrix of values.
  // A row is in the range [0:6] and a col is in the range [0:5].
  // A value is either:
  //    0 (empty),
  //    1 (occupied by Player 1),
  // or 2 (occupied by Player 2).

  Board() { clear(); }

  void set_value(std::size_t row, std::size_t col, uint8_t value) {
    assert(row < 6);
    assert(col < 7);
    const std::size_t index = row * 7 + col;
    assert(index < kNumValues);

    const std::size_t byte_index = index / kValuesPerByte;
    const std::size_t bit_offset = (index % kValuesPerByte) * kBitsPerValue;
    data_[byte_index] &= ~(0b11 << bit_offset);           // Clear existing bits
    data_[byte_index] |= ((value & 0b11) << bit_offset);  // Set new value
  }

  std::uint8_t get_value(std::size_t row, std::size_t col) const {
    assert(row < 6);
    assert(col < 7);
    const std::size_t index = row * 7 + col;
    assert(index < kNumValues);

    const std::size_t byte_index = index / kValuesPerByte;
    const std::size_t bit_offset = (index % kValuesPerByte) * kBitsPerValue;
    const std::uint8_t result = (data_[byte_index] >> bit_offset) & 0b11;
    return result;
  }

  void drop(std::size_t column);

  void clear() {
    std::fill(data_.begin(), data_.end(), 0);
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
  static constexpr std::size_t kNumValues = 42;
  static constexpr std::size_t kBitsPerValue = 2;
  static constexpr std::size_t kValuesPerByte = 8 / kBitsPerValue;
  static constexpr std::size_t kNumBytes =
      (kNumValues + kValuesPerByte - 1) / kValuesPerByte;

  std::array<uint8_t, kNumBytes> data_;
  uint8_t whose_turn_ = 1;
};
