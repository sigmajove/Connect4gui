#include "board.h"

#include <intrin.h>

#include <bit>
#include <cassert>
#include <cstddef>
#include <format>
#include <iostream>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#include <utility>
#include <vector>

class nullbuf : public std::streambuf {
 protected:
  int overflow(int c) override { return traits_type::not_eof(c); }
};

class nullstream : public std::ostream {
 public:
  nullstream() : std::ostream(&nbuf) {}

 private:
  nullbuf nbuf;
};

// Returns a mask with a single bit set.
inline std::uint64_t OneMask(std::size_t index) { return UINT64_C(1) << index; }

// Converts back and forth between an index (range [0:42)) and
// a (row, col) pair.
inline std::size_t Index(std::size_t row, std::size_t col) {
  assert(row < Board::kNumRows);
  assert(col < Board::kNumCols);
  return row * Board::kNumCols + col;
}

// Converts back and forth between an index (range [0:42)) and
// a (row, col) pair.
inline std::uint64_t Index(const Board::Coord &c) {
  return c.first * Board::kNumCols + c.second;
}

Board::Coord FromIndex(std::size_t index) {
  const std::size_t row = index / Board::kNumCols;
  const std::size_t col = index % Board::kNumCols;
  return std::make_pair(row, col);
}

const Board::MaskArray all_winning_masks = Board::winning_masks();

Board::PartialWins Board::ComputePartialWins() {
  PartialWins result;
  for (std::vector<std::size_t> &v : result) {
    v.reserve(16);
  }
  // Invert all_winning_masks.
  // For every 1 bit in all_winning_masks[i], append an i into the vector
  // corresponding to the position of that bit.
  for (std::size_t i = 0; i < all_winning_masks.size(); ++i) {
    const std::uint64_t mask = all_winning_masks[i];
    for (std::size_t j = 0; j < result.size(); ++j) {
      if ((mask >> j) & 1) {
        result[j].push_back(i);
      }
    }
  }
  return result;
}

const Board::PartialWins all_partial_wins = Board::ComputePartialWins();

void Board::set_value(std::size_t row, std::size_t col, unsigned int value) {
  const std::size_t position = Index(row, col);
  const std::uint64_t mask = OneMask(position);
#if 0
  const std::vector<std::size_t> &needs_adjust = all_partial_wins[position];

  const std::size_t current_value =
      (((yellow_set_ & mask) != 0) << 1) | ((red_set_ & mask) != 0);

  if (current_value == value) {
    // No change.
    return;
  }
  if (current_value == 0 && value == 1) {
    // Increase red counts
    for (const std::size_t i : needs_adjust) {
      const auto increased = ++partial_wins_[i].red_count;
      assert(increased <= 4);
    }
  } else if (current_value == 0 && value == 2) {
    // Increase yellow counts
    for (const std::size_t i : needs_adjust) {
      const auto increased = ++partial_wins_[i].yellow_count;
      assert(increased <= 4);
    }
  } else if (current_value == 1 && value == 0) {
    // Decrease red counts
    for (const std::size_t i : needs_adjust) {
      const auto old_value = partial_wins_[i].red_count--;
      assert(old_value > 0);
    }
  } else if (current_value == 2 && value == 0) {
    // Decrease yellow counts
    for (const std::size_t i : needs_adjust) {
      const auto old_value = partial_wins_[i].yellow_count--;
      assert(old_value > 0);
    }
  } else {
    throw std::runtime_error(
        std::format("Invalid transition {} to {}", current_value, value));
  }
#endif

  std::uint64_t unmask = ~mask;
  switch (value) {
    case 0:
      red_set_ &= unmask;
      yellow_set_ &= unmask;
      break;
    case 1:
      red_set_ |= mask;
      yellow_set_ &= unmask;
      break;
    case 2:
      red_set_ &= unmask;
      yellow_set_ |= mask;
      break;
    case 3:
      red_set_ |= mask;
      yellow_set_ |= mask;
      break;
    default:
      throw std::runtime_error(std::format("Bad value {}", value));
  }
}

// Checks the consistency of partial_wins against the current
// board position.
void Board::CheckPartialWins() {
#if 0
  for (std::size_t i = 0; i < kNumFours; ++i) {
    const std::uint64_t mask = all_winning_masks[i];
    const int red_count = std::popcount(mask & red_set_);
    const int yellow_count = std::popcount(mask & yellow_set_);
    if (partial_wins_[i].red_count != red_count ||
        partial_wins_[i].yellow_count != yellow_count) {
      throw std::runtime_error(
          std::format("Invalid count at {}: expected {}, {}, actual {} {}", i,
                      red_count, yellow_count, partial_wins_[i].red_count,
                      partial_wins_[i].yellow_count));
    }
  }
#endif
}


unsigned int Board::get_value(std::size_t row, std::size_t col) const {
  const std::uint64_t mask = OneMask(Index(row, col));
  return (((yellow_set_ & mask) != 0) << 1) | ((red_set_ & mask) != 0);
}

std::vector<std::size_t> Board::legal_moves() const {
  std::vector<std::size_t> result;
  result.reserve(kNumCols);
  for (std::size_t col = 0; col < kNumCols; ++col) {
    if (get_value(kNumRows - 1, col) == 0) {
      result.push_back(col);
    }
  }
  return result;
}

std::size_t Board::drop(std::size_t column) {
  for (std::size_t row = 0; row < kNumRows; ++row) {
    if (get_value(row, column) == 0) {
      set_value(row, column, whose_turn_);
      whose_turn_ = 3 - whose_turn_;
      return row;
    }
  }
  throw std::runtime_error("Column is full");
}

void Board::set_whose_turn() {
  if ((red_set_ & yellow_set_) != 0) {
    throw std::runtime_error("red/yellow overlap");
  }
  const int red_count = std::popcount(red_set_);
  const int yellow_count = std::popcount(yellow_set_);
  if (red_count == yellow_count) {
    whose_turn_ = 1;
  } else if (red_count == yellow_count + 1) {
    whose_turn_ = 2;
  } else {
    throw std::runtime_error("red/yellow unbalanced");
  }
}

void Board::clear() {
  red_set_ = 0;
  yellow_set_ = 0;
  stack_.clear();
  whose_turn_ = 1;

  // The computer goes second unless the human presses the "Go Second"
  // button.
  favorite_ = 2;

#if 0
  for (PieceCounts &count : partial_wins_) {
    count.red_count = 0;
    count.yellow_count = 0;
  }
#endif
}

void Board::push(std::size_t column) {
  for (std::size_t row = 0; row < kNumRows; ++row) {
    if (get_value(row, column) == 0) {
      set_value(row, column, whose_turn_);
      stack_.emplace_back(row, column);
      whose_turn_ = 3 - whose_turn_;
      return;
    }
  }
  assert(false);
}

void Board::pop() {
  assert(!stack_.empty());
  whose_turn_ = 3 - whose_turn_;
  const auto [row, col] = stack_.back();
  set_value(row, col, 0);
  stack_.pop_back();
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

Board::MaskArray Board::winning_masks() {
  Board::MaskArray result;
  std::size_t i = 0;
  combos([&i, &result](Coord a, Coord b, Coord c, Coord d) {
    std::uint64_t mask = 0;
    mask |= OneMask(Index(a));
    mask |= OneMask(Index(b));
    mask |= OneMask(Index(c));
    mask |= OneMask(Index(d));

    result[i++] = mask;
  });
  assert(i == result.max_size());

  return result;
}

Board::Outcome Board::IsGameOver() const {
  Outcome result = Outcome::kDraw;
  for (std::uint64_t mask : all_winning_masks) {
    if ((red_set_ & mask) == mask) {
      result = Outcome::kRedWins;
      break;
    }
    if ((yellow_set_ & mask) == mask) {
      result = Outcome::kYellowWins;
      break;
    }
    if ((red_set_ & mask) == 0 || (yellow_set_ & mask) == 0) {
      result = Outcome::kContested;
    }
  }

  return result;
}

// For debugging
std::string DumpMask(std::uint64_t mask) {
  std::ostringstream stream;
  for (std::size_t r = 0; r < Board::kNumRows; ++r) {
    const size_t row = Board::kNumRows - 1 - r;
    for (std::size_t col = 0; col < Board::kNumCols; ++col) {
      const std::size_t index = row * Board::kNumCols + col;
      const bool value = mask & OneMask(index);
      stream << (value ? '*' : '.');
    }
    stream << '\n';
  }
  return stream.str();
}

std::pair<std::size_t, Board::ThreeKind> Board::ThreeInARow(
    std::uint8_t me) const {
  std::uint64_t my_bits, his_bits;
  switch (me) {
    case 1:
      my_bits = red_set_;
      his_bits = yellow_set_;
      break;
    case 2:
      my_bits = yellow_set_;
      his_bits = red_set_;
      break;
    default:
      throw std::runtime_error(std::format("Bad value {}", me));
  }

  unsigned int other_count = 0;  // The number of supported three-in-a-kinds
                                 // my opponent has.
  unsigned long other_index;     // The index of the last one seen.

  // A single bitmap indicating whether either color is present.
  const std::uint64_t either_bits = red_set_ | yellow_set_;

  for (std::uint64_t mask : all_winning_masks) {
    // Check if I have a supported three-in-a-row
    const std::uint64_t my_three = mask & my_bits;
    if (__popcnt64(my_three) == 3) {
      // Find which of the four bits is turned off.
      unsigned long bit_pos;
      _BitScanForward64(&bit_pos, my_three ^ mask);

      // Check that the zero bit really is zero.
      if ((OneMask(bit_pos) & his_bits) == 0) {
        // Check if the zero bit is supported.
        if (bit_pos < kNumCols || (OneMask(bit_pos - kNumCols) & either_bits)) {
          // Return the column of the winning move.
          return std::make_pair(bit_pos % kNumCols, ThreeKind::kWin);
        }
      }
    }
    // Check if he has a supported three-in-a-row
    const std::uint64_t his_three = mask & his_bits;
    if (__popcnt64(his_three) == 3) {
      // Find which of the four bits is turned off.
      unsigned long bit_pos;
      _BitScanForward64(&bit_pos, his_three ^ mask);
      // Check that the zero bit really is zero.
      if ((OneMask(bit_pos) & my_bits) == 0) {
        // Check if the zero bit is supported.
        if (bit_pos < kNumCols || (OneMask(bit_pos - kNumCols) & either_bits)) {
          other_index = bit_pos;
          ++other_count;
        }
      }
    }
  }

  switch (other_count) {
    case 0:
      return std::make_pair(0, ThreeKind::kNone);
    case 1:
      return std::make_pair(other_index % kNumCols, ThreeKind::kBlock);
    default:
      return std::make_pair(other_index % kNumCols, ThreeKind::kLose);
  }
}

int Board::heuristic() const {
  const unsigned int other = 3 - favorite_;
  bool four_for_me = false;
  bool four_for_him = false;
  int score = 0;
  combos([other, &four_for_me, &four_for_him, &score, this](Coord a, Coord b,
                                                            Coord c, Coord d) {
    const unsigned int ac = get_value(a.first, a.second);
    const unsigned int bc = get_value(b.first, b.second);
    const unsigned int cc = get_value(c.first, c.second);
    const unsigned int dc = get_value(d.first, d.second);

    const int mine = (ac == favorite_) + (bc == favorite_) + (cc == favorite_) +
                     (dc == favorite_);
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
    }
    if (theirs > 0) {
      score -= theirs;
    }
  });

  return four_for_me ? 1000 : four_for_him ? -1000 : score;
}

std::size_t Board::find_move(std::size_t depth) {
  int alpha = std::numeric_limits<int>::min();
  const int beta = std::numeric_limits<int>::max();
  int value = std::numeric_limits<int>::min();

  // The value returned if there are no legal moves.
  std::size_t best_move = std::numeric_limits<std::size_t>::max();

  const std::vector<std::size_t> moves(legal_moves());
  for (const std::size_t col : moves) {
    push(col);
    const int child = depth == 0
                          ? heuristic()
                          : alpha_beta_helper(depth - 1, alpha, beta, false);
    pop();
    if (child > value) {
      value = child;
      best_move = col;
    }
    if (value > alpha) {
      alpha = value;
    }
  }
  return best_move;
}

// See
// https://en.wikipedia.org/wiki/Alpha%E2%80%93beta_pruning#Improvements_over_naive_minimax
int Board::alpha_beta_helper(std::size_t depth, int alpha, int beta,
                             bool maximizing) {
  const std::vector<std::size_t> moves(legal_moves());
  if (depth == 0 || moves.empty()) {
    return heuristic();
  }
  switch (IsGameOver()) {
    case Outcome::kRedWins:
      return favorite_ == 1 ? 1000 : -1000;
    case Outcome::kYellowWins:
      return favorite_ == 2 ? 1000 : -1000;
    case Outcome::kDraw:
      return 0;
    case Outcome::kContested:
      break;
  }

  if (maximizing) {
    int value = std::numeric_limits<int>::min();
    for (const std::size_t col : moves) {
      push(col);
      const int child = alpha_beta_helper(depth - 1, alpha, beta, false);
      pop();

      if (child > value) {
        value = child;
      }
      if (value >= beta) {
        break;
      }
      if (value > alpha) {
        alpha = value;
      }
    }
    return value;
  } else {
    int value = std::numeric_limits<int>::max();
    for (const std::size_t col : moves) {
      push(col);
      const int child = alpha_beta_helper(depth - 1, alpha, beta, true);
      pop();

      if (child < value) {
        value = child;
      }
      if (value <= alpha) {
        break;
      }
      if (value < beta) {
        beta = value;
      }
    }
    return value;
  }
}

Board::BruteForceResult Board::Reverse(BruteForceResult outcome) const {
  switch (outcome) {
    case BruteForceResult::kWin:
      return BruteForceResult::kLose;
    case BruteForceResult::kLose:
      return BruteForceResult::kWin;
    case BruteForceResult::kDraw:
      return BruteForceResult::kDraw;
    default:
      throw std::runtime_error(
          std::format("Bad outcome {}", static_cast<int>(outcome)));
  }
}

const char *DebugImage(Board::ThreeKind c) {
  switch (c) {
    case Board::ThreeKind::kNone:
      return "None";
    case Board::ThreeKind::kWin:
      return "Win";
    case Board::ThreeKind::kBlock:
      return "Block";
    case Board::ThreeKind::kLose:
      return "Lose";
    default:
      return "bad";
  }
}

const char *DebugImage(Board::BruteForceResult c) {
  switch (c) {
    case Board::BruteForceResult::kWin:
      return "Win";
    case Board::BruteForceResult::kDraw:
      return "Block";
    case Board::BruteForceResult::kLose:
      return "Lose";
    default:
      return "bad";
  }
}

std::string DebugImage(std::vector<std::size_t> v) {
  if (v.empty()) {
    return "<empty>";
  }
  std::ostringstream oss;
  bool need_pad = false;
  for (const std::size_t item : v) {
    if (need_pad) {
      oss << " ";
    } else {
      need_pad = true;
    }
    oss << item;
  }
  return oss.str();
}

static int counter = 0;
std::pair<Board::BruteForceResult, std::vector<size_t>> Board::BruteForce(
    double budget, std::uint8_t me) {
  const int id = ++counter;
  const auto [move, outcome] = ThreeInARow(me);
  switch (outcome) {
    case ThreeKind::kNone:
      break;
    case ThreeKind::kWin: {
      return std::make_pair(BruteForceResult::kWin,
                            std::vector<std::size_t>(1, move));
    }
    case ThreeKind::kLose: {
      return std::make_pair(BruteForceResult::kLose,
                            std::vector<std::size_t>(1, move));
    }
    case ThreeKind::kBlock: {
      push(move);
      auto [forced, path] = BruteForce(budget, 3 - me);
      pop();
      path.insert(path.begin(), move);
      if (stack_.empty()) {
        std::cout << "Evaluated " << counter << " boards\n";
      }
      return std::make_pair(Reverse(forced), path);
    }
    default:
      throw std::runtime_error("Bad outcome value");
  }
  const auto moves = legal_moves();
  if (moves.empty()) {
    return std::make_pair(BruteForceResult::kDraw, std::vector<std::size_t>());
  }
  if (budget < 1.0) {
    throw std::runtime_error(
        std::format("Ran out of budget with {} pieces placed", HowFull()));
  }
  budget = (budget - 1) / moves.size();
  BruteForceResult best = BruteForceResult::kWin;
  std::vector<size_t> best_path;
  std::size_t best_move = 9999;
  for (const std::size_t move : moves) {
    push(move);
    const auto [eval, path] = BruteForce(budget, 3 - me);
    pop();
    if (eval >= best) {
      best = eval;
      best_path = path;
      best_move = move;
    }
  }
  best_path.insert(best_path.begin(), best_move);
  if (stack_.empty()) {
    std::cout << "Evaluated " << counter << " boards\n";
  }
  return std::make_pair(Reverse(best), best_path);
}

std::pair<int, std::vector<std::size_t>> Board::alpha_beta_trace(
    std::size_t depth, int alpha, int beta, bool maximizing) {
  nullstream discard;
  std::ostream *const trace = &discard;

  const std::size_t xxxx = 10 - depth;
  assert(xxxx >= 0);
  const std::string indent(4 * xxxx, ' ');

  *trace << indent << "Start " << (maximizing ? "Max " : "Min") << "\n";
  const std::vector<std::size_t> moves(legal_moves());
  if (depth == 0 || moves.empty()) {
    const int v = heuristic();
    *trace << std::format("{}return Leaf value {}\n", indent, v);
    return std::make_pair(v, std::vector<std::size_t>());
  }
  if (maximizing) {
    std::vector<std::size_t> best_path;
    int value = std::numeric_limits<int>::min();
    for (const std::size_t col : moves) {
      push(col);
      *trace << std::format("{}Push {}\n", indent, col);
      auto [child, path] = alpha_beta_trace(depth - 1, alpha, beta, false);
      pop();
      *trace << indent << "pop eval " << child << "\n";

      if (child > value) {
        value = child;
        path.insert(path.begin(), col);
        best_path = std::move(path);
      }
      if (value >= beta) {
        *trace << indent << "Prune beta " << beta << "\n";
        break;
      }
      if (value > alpha) {
        alpha = value;
      }
    }
    *trace << std::format("{}return max value {} col {}\n", indent, value,
                          best_path[0]);
    return std::make_pair(value, std::move(best_path));
  } else {
    int value = std::numeric_limits<int>::max();
    std::vector<std::size_t> best_path;
    for (const std::size_t col : moves) {
      push(col);
      *trace << std::format("{}Push {}\n", indent, col);
      auto [child, path] = alpha_beta_trace(depth - 1, alpha, beta, true);
      pop();
      *trace << indent << "pop eval " << child << "\n";

      if (child < value) {
        value = child;
        path.insert(path.begin(), col);
        best_path = std::move(path);
      }
      if (value <= alpha) {
        *trace << indent << "Prune alpha " << alpha << "\n";
        break;
      }
      if (value < beta) {
        beta = value;
      }
    }
    *trace << std::format("{}return min value {} col {}\n", indent, value,
                          best_path[0]);
    return std::make_pair(value, std::move(best_path));
  }
}

std::string Board::image() const {
  std::ostringstream stream;
  for (std::size_t r = 0; r < kNumRows; ++r) {
    const size_t row = kNumRows - 1 - r;
    for (std::size_t col = 0; col < kNumCols; ++col) {
      const auto value = get_value(row, col);
      stream << ((value == 1) ? '1' : (value == 2) ? '2' : '.');
    }
    stream << '\n';
  }
  return stream.str();
}
