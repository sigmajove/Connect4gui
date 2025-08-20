#include "board.h"

#include <bit>
#include <cassert>
#include <compare>
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

#include "cache.h"

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

std::string MaskImage(Board::BoardMask mask) {
  std::ostringstream stream;
  bool needs_comma = false;
  for (;;) {
    if (std::popcount(mask) == 0) {
      return stream.str();
    }
    const int offset = std::countr_zero(mask);
    if (needs_comma) {
      stream << ", ";
    } else {
      needs_comma = true;
    }

    stream << std::format("Row {} Col {}", offset / Board::kNumCols,
                          offset % Board::kNumCols);
    mask &= ~OneMask(offset);
  }
}

// Returns the leftmost column in the mask, 999 if the mask is empty.
std::size_t MaskColumn(Board::BoardMask mask) {
  std::size_t result = 999;
  for (;;) {
    if (std::popcount(mask) == 0) {
      return result;
    }
    const int offset = std::countr_zero(mask);
    const std::size_t column = offset % Board::kNumCols;
    if (column < result) {
      result = column;
    }

    mask &= ~OneMask(offset);
  }
}

// Converts back and forth between an index (range [0:42)) and
// a (row, col) pair.
inline std::size_t Index(std::size_t row, std::size_t col) {
  assert(row < Board::kNumRows);
  assert(col < Board::kNumCols);
  return row * Board::kNumCols + col;
}

// Converts back and forth between an index (range [0:42)) and
// a (row, col) pair.
inline std::size_t Index(const Board::Coord &c) {
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
  // For every 1 bit in all_winning_masks[i], append the mask into the vector
  // corresponding to the position of that bit.
  for (const BoardMask mask : all_winning_masks) {
    for (std::size_t j = 0; j < result.size(); ++j) {
      if ((mask >> j) & 1) {
        result[j].push_back(mask);
      }
    }
  }
  return result;
}

const Board::PartialWins all_partial_wins = Board::ComputePartialWins();

void Board::set_value(std::size_t row, std::size_t col, unsigned int value) {
  const std::size_t position = Index(row, col);
  const BoardMask mask = OneMask(position);

  BoardMask unmask = ~mask;
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

unsigned int Board::get_value(std::size_t row, std::size_t col) const {
  const BoardMask mask = OneMask(Index(row, col));
  return (((yellow_set_ & mask) != 0) << 1) | ((red_set_ & mask) != 0);
}

// Returns the number of legal moves, and writes them into moves.
// More efficient than returning a vector, and it matters.
std::size_t Board::LegalMoves(std::size_t (&moves)[Board::kNumCols]) {
  const Board::BoardMask all_bits = red_set_ | yellow_set_;
  static constexpr std::size_t offset =
      (Board::kNumRows - 1) * (Board::kNumCols);
  std::size_t count = 0;
  for (std::size_t col = 0; col < Board::kNumCols; ++col) {
    if ((all_bits & OneMask(offset + col)) == 0) {
      moves[count++] = col;
    }
  }
  return count;
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

// Given a board position, decide whose turn it is.
// Returns 1 for red and 2 for yellow.
unsigned int GetWhoseTurn(const Board::Position &p) {
  if ((p.red_set & p.yellow_set) != 0) {
    throw std::runtime_error("red/yellow overlap");
  }
  const int red_count = std::popcount(p.red_set);
  const int yellow_count = std::popcount(p.yellow_set);
  if (red_count == yellow_count) {
    return 1;  // red
  }
  if (red_count == yellow_count + 1) {
    return 2;  // yellow
  }
  throw std::runtime_error("red/yellow unbalanced");
}

// Given a board position, decide whose turn it is.
// Returns 1 for red and 2 for yellow.
unsigned int Board::Position::WhoseTurn() const {
  if ((red_set & yellow_set) != 0) {
    throw std::runtime_error("red/yellow overlap");
  }
  const int red_count = std::popcount(red_set);
  const int yellow_count = std::popcount(yellow_set);
  if (red_count == yellow_count) {
    return 1;  // red
  }
  if (red_count == yellow_count + 1) {
    return 2;  // yellow
  }
  throw std::runtime_error("red/yellow unbalanced");
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
  stack_size_ = 0;
  whose_turn_ = 1;

  // The computer goes second unless the human presses the "Go Second"
  // button.
  favorite_ = 2;
}

// Compute a mask with a 1 set in every row of the lefmost column.
Board::BoardMask Board::CreateColumnMask() {
  BoardMask result = 0;
  for (std::size_t index = 0; index < kBoardSize; index += kNumCols) {
    result |= OneMask(index);
  }
  return result;
}

const Board::BoardMask column_mask = Board::CreateColumnMask();

void Board::push(std::size_t column) {
  const int bit_pos =
      std::countr_zero(~(red_set_ | yellow_set_) & (column_mask << column));
  if (bit_pos > 63) {
    throw std::runtime_error("Column is full");
  }
  if (stack_size_ >= kBoardSize) {
    throw std::runtime_error("Stack overflow");
  }
  new_stack_[stack_size_].red_set = red_set_;
  new_stack_[stack_size_].yellow_set = yellow_set_;
  new_stack_[stack_size_].column = column;
  ++stack_size_;

  const BoardMask mask = OneMask(bit_pos);
  if (whose_turn_ == 1) {
    red_set_ |= mask;
  } else {
    yellow_set_ |= mask;
  }
  whose_turn_ = 3 - whose_turn_;
}

// Returns the number of legal moves, and writes them into moves.
// More efficient than returning a vector, and it matters.
// The moves are are represented by bit masks.
std::size_t LegalMovesM(Board::BoardMask red_set, Board::BoardMask yellow_set,
                        Board::BoardMask (&moves)[Board::kNumCols]) {
  const Board::BoardMask candidates = ~(red_set | yellow_set);
  std::size_t count = 0;
  for (std::size_t col = 0; col < Board::kNumCols; ++col) {
    const int bit_pos = std::countr_zero(candidates & (column_mask << col));
    if (bit_pos < 64) {
      moves[count++] = OneMask(bit_pos);
    }
  }
  return count;
}

void Board::pop() {
  if (stack_size_ == 0) {
    throw std::runtime_error("Stack underflow");
  }
  whose_turn_ = 3 - whose_turn_;
  --stack_size_;
  red_set_ = new_stack_[stack_size_].red_set;
  yellow_set_ = new_stack_[stack_size_].yellow_set;
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
    BoardMask mask = 0;
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
  for (BoardMask mask : all_winning_masks) {
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

Board::Outcome Board::Position::IsGameOver() const {
  Board::Outcome result = Board::Outcome::kDraw;
  for (Board::BoardMask mask : all_winning_masks) {
    if ((red_set & mask) == mask) {
      result = Outcome::kRedWins;
      break;
    }
    if ((yellow_set & mask) == mask) {
      result = Outcome::kYellowWins;
      break;
    }
    if ((red_set & mask) == 0 || (yellow_set & mask) == 0) {
      result = Outcome::kContested;
    }
  }

  return result;
}

// For debugging
std::string DumpMask(Board::BoardMask mask) {
  std::ostringstream stream;
  for (std::size_t r = 0; r < Board::kNumRows; ++r) {
    const size_t row = Board::kNumRows - 1 - r;
    for (std::size_t col = 0; col < Board::kNumCols; ++col) {
      const std::size_t index = row * Board::kNumCols + col;
      const bool value = mask & OneMask(index);
      stream << (value ? '%' : '.');
    }
    stream << '\n';
  }
  return stream.str();
}

Board::BoardMask FindTriples(const Board::BoardMask &board) {
  Board::BoardMask result = 0;
  for (const Board::BoardMask mask : all_winning_masks) {
    const Board::BoardMask four_bits = mask & board;
    if (std::popcount(four_bits) == 3) {
      // Find the hole in the three bits.
      result |= OneMask(std::countr_zero(four_bits ^ mask));
    }
  }
  return result;
}

// Finds all occurences of three of four bits in board.
// Add the mask for the missing fourth bit into the result.
Board::BoardMask FindNewTriples(const Board::BoardMask &board,
                                Board::BoardMask move) {
  Board::BoardMask result = 0;
  for (const Board::BoardMask mask : all_partial_wins[std::countr_zero(move)]) {
    const Board::BoardMask four_bits = mask & board;
    if (std::popcount(four_bits) == 3) {
      // Find the hole in the three bits.
      result |= OneMask(std::countr_zero(four_bits ^ mask));
    }
  }
  return result;
}

Board::BoardMask Board::Position::LegalMoves() const {
  // A single bitmap indicating where the empty spaces on the board are.
  const BoardMask empty_squares = ~(red_set | yellow_set);

  // Find all the places where a piece can be legally played.
  BoardMask legal_moves = 0;

  for (std::size_t col = 0; col < Board::kNumCols; ++col) {
    const int bit_pos = std::countr_zero(empty_squares & (column_mask << col));
    if (bit_pos < 64) {
      legal_moves |= OneMask(bit_pos);
    }
  }

  return legal_moves;
}

std::pair<Board::BoardMask, Board::ThreeKind> ThreeInARow(
    unsigned int me, Board::BoardMask red_triples,
    Board::BoardMask yellow_triples, Board::BoardMask legal_moves) {
  Board::BoardMask my_triples;
  Board::BoardMask his_triples;
  switch (me) {
    case 1:
      my_triples = red_triples;
      his_triples = yellow_triples;
      break;
    case 2:
      my_triples = yellow_triples;
      his_triples = red_triples;
      break;
    default:
      throw std::runtime_error(std::format("Bad value {}", me));
  }

  // See if I can win.
  if (const Board::BoardMask winners = my_triples & legal_moves; winners != 0) {
    return std::make_pair(winners, Board::ThreeKind::kWin);
  }

  const Board::BoardMask blocks = his_triples & legal_moves;
  if (blocks == 0) {
    return std::make_pair(0, Board::ThreeKind::kNone);
  }
  return std::make_pair(blocks, std::popcount(blocks) == 1
                                    ? Board::ThreeKind::kBlock
                                    : Board::ThreeKind::kLose);
}

std::pair<Board::BoardMask, Board::ThreeKind> Board::Position::ThreeInARow(
    unsigned int me) const {
  return ::ThreeInARow(me, FindTriples(red_set), FindTriples(yellow_set),
                       LegalMoves());
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

Metric Board::Reverse(Metric metric) {
  return Metric(Reverse(metric.result), metric.depth);
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
      return "Unknown Value";
  }
}

std::string DebugImage(Board::Outcome outcome) {
  switch (outcome) {
    case Board::Outcome::kContested:
      return "Contested";
    case Board::Outcome::kRedWins:
      return "Red Wins";
    case Board::Outcome::kYellowWins:
      return "Yellow Wins";
    case Board::Outcome::kDraw:
      return "Draw";
    default:
      return "Unknown Value";
  }
}

std::string DebugImage(BruteForceResult c) {
  switch (c) {
    case BruteForceResult::kWin:
      return "Win";
    case BruteForceResult::kDraw:
      return "Draw";
    case BruteForceResult::kLose:
      return "Lose";
    case BruteForceResult::kNil:
      return "Nil";
    case BruteForceResult::kInf:
      return "Inf";
    default:
      return std::format("Unknown Value ({})", static_cast<int>(c));
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

std::ostream &operator<<(std::ostream &os, const Metric &metric) {
  os << "(" << DebugImage(metric.result) << ", d=" << metric.depth << ")";
  return os;
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

std::string Board::Position::image() const {
  std::ostringstream stream;
  for (std::size_t r = 0; r < kNumRows; ++r) {
    const size_t row = kNumRows - 1 - r;
    for (std::size_t col = 0; col < kNumCols; ++col) {
      const BoardMask mask = OneMask(Index(row, col));
      switch (2 * ((mask & red_set) != 0) + ((mask & yellow_set) != 0)) {
        case 0:
          stream << '.';
          break;
        case 1:
          stream << '2';
          break;
        case 2:
          stream << '1';
          break;
        case 3:
          stream << '3';
          break;
      }
    }
    stream << '\n';
  }
  return stream.str();
}

std::string MaskMap(Board::BoardMask mask) {
  std::ostringstream stream;
  for (std::size_t r = 0; r < Board::kNumRows; ++r) {
    const size_t row = Board::kNumRows - 1 - r;
    for (std::size_t col = 0; col < Board::kNumCols; ++col) {
      if (mask & OneMask(Index(row, col))) {
        stream << "*";
      } else {
        stream << ".";
      }
    }
    stream << '\n';
  }
  return stream.str();
}

void Board::Position::set_value(std::size_t row, std::size_t col,
                                unsigned int value) {
  const BoardMask mask = OneMask(Index(row, col));
  BoardMask unmask = ~mask;
  switch (value) {
    case 0:
      red_set &= unmask;
      yellow_set &= unmask;
      break;
    case 1:
      red_set |= mask;
      yellow_set &= unmask;
      break;
    case 2:
      red_set &= unmask;
      yellow_set |= mask;
      break;
    case 3:
      red_set |= mask;
      yellow_set |= mask;
      break;
    default:
      throw std::runtime_error(std::format("Bad value {}", value));
  }
}
Board::Position Board::ParsePosition(const std::string image) {
  Board::Position b;
  if (image.size() != 42 + 6 + 1) {
    throw std::runtime_error("string size");
  }
  std::size_t i = 0;
  const auto getc = [image, &i]() { return image[i++]; };
  if (getc() != '\n') {
    throw std::runtime_error("initial newline");
  }
  for (std::size_t row = 5;; --row) {
    for (std::size_t col = 0; col < 7; ++col) {
      const Board::BoardMask mask = OneMask(Index(row, col));
      switch (getc()) {
        case '.':
          break;
        case '1':
          b.red_set |= mask;
          break;
        case '2':
          b.yellow_set |= mask;
          break;
        default:
          throw std::runtime_error("bad value");
      }
    }
    if (getc() != '\n') {
      throw std::runtime_error("line length");
    }
    if (row == 0) {
      break;
    }
  }
  return b;
}

Board::BruteForceReturn4 Board::BruteForce4(Board::Position position,
                                            double budget) {
  // The returned result.
  BoardMask best_move = 0;

  std::size_t report_count = 0;

  struct StackFrame {
    StackFrame(Position position, unsigned int whose_turn,
               BoardMask legal_moves, BoardMask red_triples,
               BoardMask yellow_triples, Metric cutoff, Metric accum)
        : position(position),
          whose_turn(whose_turn),
          legal_moves(legal_moves),
          best(BruteForceResult::kNil, 0),  // Negative infinity.
          red_triples(red_triples),
          yellow_triples(yellow_triples),
          cutoff(cutoff),
          accum(accum) {}

    // Input parameters
    double budget;
    Position position;

    // Redundant with position.
    unsigned int whose_turn;  // 1 or 2

    BoardMask legal_moves;

    BoardMask moves[kNumCols];
    std::size_t num_moves;
    std::size_t current_move = 0;

    Metric best;

    BoardMask red_triples, yellow_triples;

    // Otherwise known as the alpha and beta in Alpha-beta pruning.
    // Alpha-beta pruning significantly speeds up the search algorithm.
    // We use a variation on the classic algorithm found at
    // https://en.wikipedia.org/wiki/Alpha-beta_pruning#Pseudocode
    // so that we can use the same code to evaluate the position of
    // either player.
    Metric cutoff, accum;
  };
  std::vector<StackFrame> restack;  // The recursion stack.

  const auto stack_trace = [&restack]() {
    std::cout << "**** Stack Trace ****\n";
    for (std::size_t i = 0; i < restack.size(); ++i) {
      const StackFrame &top = restack[i];
      std::cout << std::format("Level {} Player {}\n{}-------------\n", i,
                               top.whose_turn, top.position.image());
    }
  };

  try {
    // This variable is read at report_result.
    Metric result;

    // These variables are read at the beginning of the loop.
    // They should not be referenced elsewhere.
    Board::Position new_pos = position;
    unsigned int new_whose_turn = GetWhoseTurn(new_pos);
    BoardMask new_legal_moves = position.LegalMoves();
    double new_budget = budget;
    BoardMask new_red_triples = FindTriples(position.red_set);
    BoardMask new_yellow_triples = FindTriples(position.yellow_set);

    Metric new_cutoff(BruteForceResult::kInf, 0);  // Negative infinity
    Metric new_accum(BruteForceResult::kNil, 0);   // Positive infinity.

    for (;;) {
      {
        // Evaluate new_pos, new_whose_turn, new_budget
        // If new_pos is warranted, a new stack frame is created,
        // and the input values are used to create it.

        Board::BoardMask my_triples;
        Board::BoardMask his_triples;
        switch (new_whose_turn) {
          case 1:
            my_triples = new_red_triples;
            his_triples = new_yellow_triples;
            break;
          case 2:
            my_triples = new_yellow_triples;
            his_triples = new_red_triples;
            break;
          default:
            throw std::runtime_error(
                std::format("Bad value {}", new_whose_turn));
        }

        // See if I can win.
        if (const BoardMask winning_move = my_triples & new_legal_moves;
            winning_move != 0) {
          if (restack.empty()) {
            return Board::BruteForceReturn4(BruteForceResult::kWin,
                                            winning_move);
          }

          // Reverse the polarity.
          result.result = BruteForceResult::kLose;
          result.depth = restack.size();
          goto report_result;
        }

        // See if I have a forced block or loss
        const BoardMask move = his_triples & new_legal_moves;
        if (move == 0 || std::popcount(move) == 1) {
          // None or Block
          if (new_budget < 1.0) {
            throw std::runtime_error(std::format("Ran out of budget"));
          }
          restack.emplace_back(new_pos, new_whose_turn, new_legal_moves,
                               new_red_triples, new_yellow_triples, new_cutoff,
                               new_accum);
          StackFrame &top = restack.back();

          // Initialize top.num_moves and top.moves.
          if (move == 0) {
            top.num_moves =
                LegalMovesM(new_pos.red_set, new_pos.yellow_set, top.moves);
          } else {
            top.num_moves = 1;
            top.moves[0] = move;
          }
          top.budget = (new_budget - 1) / top.num_moves;
          goto advance_top;
        }

        // Lose
        if (restack.empty()) {
          return Board::BruteForceReturn4(BruteForceResult::kLose, move);
        }

        // Reverse the polarity.
        result.result = BruteForceResult::kWin;
        result.depth = restack.size();
        // Fall into report_result
      }

    report_result: {
      StackFrame &top = restack.back();
      if (top.current_move == 0) {
        throw std::runtime_error(std::format("current move equals zero"));
      }
      const BoardMask move = top.moves[top.current_move - 1];
      switch (compare(result, top.best)) {
        case 1:  // result is better
          top.best = result;
          if (restack.size() == 1) {
            best_move = move;
          }

          // Don't bother updating top.cutoff and top.accum if we are about
          // to pop the stack.

          // We cannot apply the Alpha/Beta optimization at Level 2.
          // If we did, we would correctly determine who wins, but
          // at Level 1 we could produce wrong winning moves.
          if (restack.size() > 2 && top.current_move < top.num_moves) {
            if (compare(result, top.cutoff) >= 0) {
              if (restack.size() == 1) {
                throw std::runtime_error("Cutoff at level one");
              }

              result.result = Reverse(result.result);
              restack.pop_back();
              ++report_count;
              goto report_result;
            }
            if (compare(result, top.accum) > 0) {
              top.accum = result;
            }
          }
          break;
        case 0:  // Both are the same
          if (restack.size() == 1) {
            best_move |= move;
          }
        case -1:  // top.best is better
          break;
        default:
          throw std::runtime_error("Bad compare");
      }
      // Fall into advance_top.
    }

    advance_top: {
      if (restack.empty()) {
        throw std::runtime_error("Stack empty");
      }
      StackFrame &top = restack.back();
      if (top.current_move >= top.num_moves) {
        if (top.best.result == BruteForceResult::kNil) {
          // There were no legal moves.
          top.best.result = BruteForceResult::kDraw;
          top.best.depth = restack.size();
        }
        if (restack.size() == 1) {
          return Board::BruteForceReturn4(top.best.result, best_move);
        }

        result.result = Reverse(top.best.result);
        result.depth = top.best.depth;
        restack.pop_back();
        ++report_count;
        goto report_result;
      }

      // Get the next move.
      const BoardMask move = top.moves[top.current_move++];

      // Apply the next move to to top.position to create a new board
      // position. Initialize new_pos, new_whose_turn, and new_budget,
      // so that we can loop back to evaluate this new position.
      new_pos = top.position;
      if (top.whose_turn != new_pos.WhoseTurn()) {
        throw std::runtime_error("turn out of whack\n");
      }
#if 0
      std::cout << "Player " << top.whose_turn << " plays at "
                << MaskImage(move) << "\nBoard:\n"
                << new_pos.image();
#endif
      if (top.whose_turn == 1) {
        new_pos.red_set |= move;
        new_red_triples =
            top.red_triples | FindNewTriples(new_pos.red_set, move);
        new_yellow_triples = top.yellow_triples;
      } else {
        new_pos.yellow_set |= move;
        new_yellow_triples =
            top.yellow_triples | FindNewTriples(new_pos.yellow_set, move);
        new_red_triples = top.red_triples;
      }

      // Update legal_moves to reflect the move just made.
      static constexpr BoardMask kMoveLimit = OneMask(kBoardSize);
      new_legal_moves = top.legal_moves & ~(move);
      if (const BoardMask next_move = move << kNumCols;
          next_move < kMoveLimit) {
        new_legal_moves |= next_move;
      }

      new_whose_turn = 3 - top.whose_turn;
      if (new_whose_turn != new_pos.WhoseTurn()) {
        throw std::runtime_error("whose turn?");
      }
      new_budget = top.budget;

      // Swap cutoff and accum
      new_cutoff = Reverse(top.accum);
      new_accum = Reverse(top.cutoff);
    }
    }
  } catch (const std::exception &e) {
    std::cout << "Exception " << e.what() << "\n";
    stack_trace();
    throw;
  } catch (...) {
    std::cout << "Unknown exception\n";
    stack_trace();
    throw;
  }
}
