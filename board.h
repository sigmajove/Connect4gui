#pragma once

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <string>
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

  void set_favorite(uint8_t player) {
    assert(player == 1 || player == 2);
    favorite_ = player;
  }

  uint8_t favorite() const { return favorite_; }

  // Just compares the values of the squares.
  friend bool operator==(const Board& lhs, const Board& rhs) {
    return lhs.red_set_ == rhs.red_set_ && lhs.yellow_set_ == rhs.yellow_set_;
  }

  // Set and Get functions.
  void set_value(std::size_t row, std::size_t col, uint8_t value);
  std::uint8_t get_value(std::size_t row, std::size_t col) const;

  // Returns the columns where drop and push are valid.
  std::vector<std::size_t> legal_moves() const;

  // Returns the row into which the checker is dropped.
  std::size_t drop(std::size_t column);

  void push(std::size_t column);
  void pop();

  void clear() {
    red_set_ = 0;
    yellow_set_ = 0;
    stack_.clear();
    whose_turn_ = 1;

    // The computer goes second unless the human presses the "Go Second" button.
    favorite_ = 2;
  }

  enum class Outcome { kContested, kRedWins, kYellowWins, kDraw };
  Outcome IsGameOver() const;

  // Estimate the desirability of the board from the point of view
  // of the favorite().
  // Score 1000 points if the player has four in a row.
  // Score -1000 points if the opponent has four in a row.
  // For each combo with only player's tokens, score 1, 2, or 3
  // points depending on the number of tokens.
  // For each combo with only the opponent's tokens, score -1, -2, or -3,
  // depending on the number of tokens.
  int heuristic() const;

  using Coord = std::pair<std::size_t, std::size_t>;

  enum class ThreeKind {
    kNone,   // Nobody has a supported three-in-a-row. column is irrelevant.
    kWin,    // I have a supported three-in-a-row. column is the winning move.
    kBlock,  // column required to block opponants's supported three-in-a-row.
    kLose,   // Opponent has two supported three-in-a-rows. I will lose,
             // because I can only block one.
  };

  // Searches for supported three-in-a-rows. "Supported" means the fourth
  // square is empty, and the square below it is occupied or nonexistent.
  // If found, returns the column needed to make or block four-in-a-row.
  std::pair<std::size_t, ThreeKind> ThreeInARow(std::uint8_t me) const;

  // Calls visit with each of the four-in-a-row coordinate possibilities.
  static void combos(
      std::function<void(Coord a, Coord b, Coord c, Coord d)> visit);

  // Uses alpha-beta-minimax to find the best possible move using the
  // search depth.
  std::size_t find_move(std::size_t depth);

  // For debugging.
  std::pair<int, std::vector<std::size_t>> alpha_beta_trace(std::size_t depth,
                                                            int alpha, int beta,
                                                            bool maximizing);

  // 69 is the number of possible 4-in-a-row positions on the board.
  using MaskArray = std::array<std::uint64_t, 69>;

  // Computes the winning masks at program startup.
  static MaskArray winning_masks();

  // Returns a string representation of the board.
  std::string image() const;

 private:
  // The recursive function that performs alpha-beta minimax restricted
  // to the given depth.
  int alpha_beta_helper(std::size_t depth, int alpha, int beta,
                        bool maximizing);

  static constexpr std::size_t kNumValues = kNumRows * kNumCols;
  static constexpr std::size_t kBitsPerValue = 2;
  static constexpr std::size_t kValuesPerByte = 8 / kBitsPerValue;
  static constexpr std::size_t kNumBytes =
      (kNumValues + kValuesPerByte - 1) / kValuesPerByte;

  // Each of these is 48 bits, numbered rowwise.
  // "red" is player 1 and "yellow" is player 2.
  std::uint64_t red_set_ = 0;
  std::uint64_t yellow_set_ = 0;

  std::vector<Coord> stack_;
  uint8_t whose_turn_ = 1;

  // The payer we want to win.
  uint8_t favorite_ = 1;
};
