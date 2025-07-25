#pragma once

#include <intrin.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
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
  static constexpr std::size_t kBoardSize = kNumRows * kNumCols;

  // A (row, col) pair.
  using Coord = std::pair<std::size_t, std::size_t>;

  Board() { clear(); }

  Board(const Board&) = default;
  Board& operator=(const Board&) = default;

  // Sets the player that the move-finder wants to win.
  void set_favorite(uint8_t player) {
    assert(player == 1 || player == 2);
    favorite_ = player;
  }

  // Returns the number of pieces on the board.
  std::size_t HowFull() const { return __popcnt64(red_set_ | yellow_set_); }

  uint8_t favorite() const { return favorite_; }

  // Just compares the values of the squares.
  friend bool operator==(const Board& lhs, const Board& rhs) {
    return lhs.red_set_ == rhs.red_set_ && lhs.yellow_set_ == rhs.yellow_set_;
  }

  // Set and Get functions.
  void set_value(std::size_t row, std::size_t col, unsigned int value);
  unsigned int get_value(std::size_t row, std::size_t col) const;

  // Returns the columns where drop and push are valid.
  std::vector<std::size_t> legal_moves() const;

  // Returns the row into which the checker is dropped.
  std::size_t drop(std::size_t column);

  // Resets the board to empty.
  void clear();

  void push(std::size_t column);
  void pop();

  // Consistency check on the number of squares of each color.
  // Figure out whose turn it is based on those numbers.
  void set_whose_turn();

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

  // The number of possible 4-in-a-row positions on the board.
  static constexpr std::size_t kNumFours = 69;
  using MaskArray = std::array<std::uint64_t, kNumFours>;

  // Computes the winning masks at program startup.
  static MaskArray winning_masks();

  // A map from the board position to the winning_masks that include that
  // position.
  using PartialWins = std::array<std::vector<std::size_t>, kNumRows * kNumCols>;

  // Computes SomeName at program startup.
  static PartialWins ComputePartialWins();

  // For debugging. Tests the incremental maintenance of partial wins.
  void CheckPartialWins();

  // Returns a string representation of the board.
  std::string image() const;

  enum class BruteForceResult { kWin, kDraw, kLose };
  std::pair<BruteForceResult, std::vector<std::size_t>> BruteForce(
      double budget, std::uint8_t me);

 private:
  // The recursive function that performs alpha-beta minimax restricted
  // to the given depth.
  int alpha_beta_helper(std::size_t depth, int alpha, int beta,
                        bool maximizing);

  BruteForceResult Reverse(BruteForceResult outcome) const;

  static constexpr std::size_t kNumValues = kNumRows * kNumCols;
  static constexpr std::size_t kBitsPerValue = 2;
  static constexpr std::size_t kValuesPerByte = 8 / kBitsPerValue;
  static constexpr std::size_t kNumBytes =
      (kNumValues + kValuesPerByte - 1) / kValuesPerByte;

  // Each of these is 48 bits, numbered rowwise.
  // "red" is player 1 and "yellow" is player 2.
  std::uint64_t red_set_ = 0;
  std::uint64_t yellow_set_ = 0;

  // The number of pieces in each color in a partial win.
  struct PieceCounts {
    std::uint8_t red_count;
    std::uint8_t yellow_count;
  };

#if 0
  std::array<PieceCounts, kNumFours> partial_wins_;
#endif

  std::vector<Coord> stack_;
  unsigned int whose_turn_ = 1;

  // The payer we want to win.
  unsigned int favorite_ = 1;
};
