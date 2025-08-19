#pragma once

#include <stdint.h>

#include <algorithm>
#include <array>
#include <bit>
#include <cassert>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <format>
#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

// The type returned by BruteForce.
// kInf and kNil are never returned, but are used internally.
enum class BruteForceResult { kInf, kWin, kDraw, kLose, kNil };

struct Metric {
  // Any valid metric is better than this:
  Metric() : result(BruteForceResult::kNil), depth(0) {}

  Metric(BruteForceResult result, std::size_t depth)
      : result(result), depth(depth) {}

  // Determines which Metric is better.
  // Like a spaceship operator, but the result can be used in a switch
  // statement.
  //
  // +1: lhs is better
  //  0: both are the same
  // -1: rhs is better
  friend int compare(const Metric& lhs, const Metric& rhs) {
    const std::strong_ordering compare_result = lhs.result <=> rhs.result;
    if (compare_result == std::strong_ordering::less) {
      return 1;  // lhs is better
    }
    if (compare_result == std::strong_ordering::equal) {
      // Break ties with stack depth.
      // Note: winners want to win as soon as possible,
      // but losers want to delay the loss as much as possible.
      const std::strong_ordering compare_depth = lhs.depth <=> rhs.depth;
      switch (lhs.result) {
        case BruteForceResult::kWin:
          return compare_depth == std::strong_ordering::less    ? 1
                 : compare_depth == std::strong_ordering::equal ? 0
                                                                : -1;
        case BruteForceResult::kLose:
          return compare_depth == std::strong_ordering::less    ? -1
                 : compare_depth == std::strong_ordering::equal ? 0
                                                                : 1;
        default:
          return 0;  // Stack depth is ignored for ties (and kNil?).
                     // The depth of ties should all be the same, anyway,
      }
    }
    return -1;  // rhs is better
  }

  BruteForceResult result;
  std::size_t depth;
};

std::ostream& operator<<(std::ostream& os, const Metric& metric);

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

  // A 42-bit value representing a board postion.
  // The first 7 bits are the bottom row, lo to hi for left to right.
  // the next 7 bits are the second from the bottom row, etc.
  using BoardMask = std::uint64_t;

  // A (row, col) pair.
  using Coord = std::pair<std::size_t, std::size_t>;

  enum class Outcome { kContested, kRedWins, kYellowWins, kDraw };

  enum class ThreeKind {
    kNone,   // Nobody has a supported three-in-a-row. column is irrelevant.
    kWin,    // I have a supported three-in-a-row. column is the winning move.
    kBlock,  // column required to block opponants's supported three-in-a-row.
    kLose,   // Opponent has two supported three-in-a-rows. I will lose,
             // because I can only block one.
  };

  struct Position {
    Position& operator=(const Position&) = default;

    void set_value(std::size_t row, std::size_t col, unsigned int value);
    Board::Outcome IsGameOver() const;
    unsigned int WhoseTurn() const;

    // Searches for supported three-in-a-rows. "Supported" means the fourth
    // square is empty, and the square below it is occupied or nonexistent.
    // If found, returns the moves needed to make or block four-in-a-row.
    // Returns zero for the move if no supported three-in-a-rows are found.
    std::pair<Board::BoardMask, Board::ThreeKind> ThreeInARow(
        unsigned int me) const;

    std::string image() const;

    // Each of these is 48 bits, numbered rowwise.
    // "red" is player 1 and "yellow" is player 2.
    BoardMask red_set = 0;
    BoardMask yellow_set = 0;
  };

  static Position ParsePosition(const std::string image);

  Board() { clear(); }

  Board(const Board&) = default;
  Board& operator=(const Board&) = default;

  // Sets the player that the move-finder wants to win.
  void set_favorite(uint8_t player) {
    assert(player == 1 || player == 2);
    favorite_ = player;
  }

  // Returns the number of pieces on the board.
  int HowFull() const { return std::popcount(red_set_ | yellow_set_); }

  uint8_t favorite() const { return favorite_; }
  unsigned int whose_turn() const { return whose_turn_; }

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

  // Calls visit with each of the four-in-a-row coordinate possibilities.
  static void combos(
      std::function<void(Coord a, Coord b, Coord c, Coord d)> visit);

  // Uses alpha-beta-minimax to find the best possible move using the
  // search depth.
  std::size_t find_move(std::size_t depth);

  // The number of possible 4-in-a-row positions on the board.
  static constexpr std::size_t kNumFours = 69;
  using MaskArray = std::array<BoardMask, kNumFours>;

  // Computes the winning masks at program startup.
  static MaskArray winning_masks();

  static BoardMask CreateColumnMask();

  // A map from the board position to the winning_masks that include that
  // position.
  using PartialWins = std::array<std::vector<std::size_t>, kNumRows * kNumCols>;

  // Computes SomeName at program startup.
  static PartialWins ComputePartialWins();

  // For debugging. Tests the incremental maintenance of partial wins.
  void CheckPartialWins();

  // Returns a string representation of the board.
  std::string image() const;

  std::string HexImage() const {
    return std::format("{:011x}-{:011x}", red_set_, yellow_set_);
  }

  std::size_t LegalMoves(std::size_t (&moves)[Board::kNumCols]);

  struct BruteForceReturn4 {
    BruteForceReturn4(BruteForceResult result, BoardMask move)
        : result(result), move(move) {}

    // Need this to allocate one, I guess.
    // The values are bogus.
    BruteForceReturn4() : result(BruteForceResult::kDraw), move(0) {}

    BruteForceResult result;
    BoardMask move;
  };

  static BruteForceReturn4 BruteForce4(Board::Position position, double budget);

 private:
  // The recursive function that performs alpha-beta minimax restricted
  // to the given depth.
  int alpha_beta_helper(std::size_t depth, int alpha, int beta,
                        bool maximizing);

  static BruteForceResult Reverse(BruteForceResult result);
  static Metric Reverse(Metric metric);

  static constexpr std::size_t kNumValues = kNumRows * kNumCols;
  static constexpr std::size_t kBitsPerValue = 2;
  static constexpr std::size_t kValuesPerByte = 8 / kBitsPerValue;
  static constexpr std::size_t kNumBytes =
      (kNumValues + kValuesPerByte - 1) / kValuesPerByte;

  // Each of these is 48 bits, numbered rowwise.
  // "red" is player 1 and "yellow" is player 2.
  BoardMask red_set_ = 0;
  BoardMask yellow_set_ = 0;

  // The number of pieces in each color in a partial win.
  struct PieceCounts {
    std::uint8_t red_count;
    std::uint8_t yellow_count;
  };

#if 0
  std::array<PieceCounts, kNumFours> partial_wins_;
#endif

  struct Data {
    BoardMask red_set;
    BoardMask yellow_set;
    std::size_t column;  // For debugging
  };
  Data new_stack_[kBoardSize];
  std::size_t stack_size_ = 0;

  unsigned int whose_turn_ = 1;

  // The payer we want to win.
  unsigned int favorite_ = 1;
};

// Returns a mask with a single bit set.
inline Board::BoardMask OneMask(std::size_t index) {
  return UINT64_C(1) << index;
}

std::string MaskImage(Board::BoardMask mask);

// Returns the leftmost column in the mask, 999 if the mask is empty.
std::size_t MaskColumn(Board::BoardMask mask);

std::string DebugImage(Board::Outcome outcome);
std::string DebugImage(BruteForceResult c);
