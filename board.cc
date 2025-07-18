#include "board.h"

#include <cassert>
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

void Board::set_value(std::size_t row, std::size_t col, uint8_t value) {
  assert(row < kNumRows);
  assert(col < kNumCols);
  const std::size_t index = row * kNumCols + col;
  assert(index < kNumValues);
  std::uint64_t mask = UINT64_C(1) << index;
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
      throw std::runtime_error(std::format("Bad set value {}", value));
  }
}

std::uint8_t Board::get_value(std::size_t row, std::size_t col) const {
  assert(row < kNumRows);
  assert(col < kNumCols);
  const std::size_t index = row * kNumCols + col;
  assert(index < kNumValues);

  std::uint64_t mask = UINT64_C(1) << index;

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
  const auto [row, col] = stack_.back();
  set_value(row, col, 0);
  whose_turn_ = 3 - whose_turn_;
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

std::uint64_t Index(const Board::Coord &c) {
  return c.first * Board::kNumCols + c.second;
}

Board::MaskArray Board::winning_masks() {
  Board::MaskArray result;
  std::size_t i = 0;
  combos([&i, &result](Coord a, Coord b, Coord c, Coord d) {
    std::uint64_t mask = 0;
    mask |= (UINT64_C(1) << Index(a));
    mask |= (UINT64_C(1) << Index(b));
    mask |= (UINT64_C(1) << Index(c));
    mask |= (UINT64_C(1) << Index(d));

    result[i++] = mask;
  });
  assert(i == result.max_size());

  return result;
}

const Board::MaskArray all_winning_masks = Board::winning_masks();

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

int Board::heuristic() const {
  const std::uint8_t other = 3 - favorite_;
  bool four_for_me = false;
  bool four_for_him = false;
  int score = 0;
  combos([other, &four_for_me, &four_for_him, &score, this](Coord a, Coord b,
                                                            Coord c, Coord d) {
    const std::uint8_t ac = get_value(a.first, a.second);
    const std::uint8_t bc = get_value(b.first, b.second);
    const std::uint8_t cc = get_value(c.first, c.second);
    const std::uint8_t dc = get_value(d.first, d.second);

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

std::pair<int, std::vector<std::size_t> > Board::alpha_beta_trace(
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
