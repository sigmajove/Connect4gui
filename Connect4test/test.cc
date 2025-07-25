#include <cstdint>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "../board.h"
#include "gtest/gtest.h"

Board parse(const std::string image) {
  Board b;
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
      uint8_t value;
      switch (getc()) {
        case '.':
          value = 0;
          break;
        case '1':
          value = 1;
          break;
        case '2':
          value = 2;
          break;
        default:
          throw std::runtime_error("bad value");
      }
      b.set_value(row, col, value);
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

void test_in_range(Board::Coord &c) {
  EXPECT_LT(c.first, 6);
  EXPECT_LT(c.second, 7);
}

TEST(Game, Combos) {
  int counter = 0;
  Board::combos([&counter](Board::Coord a, Board::Coord b, Board::Coord c,
                           Board::Coord d) { ++counter; });
  EXPECT_EQ(counter, 69);

  Board::combos(
      [](Board::Coord a, Board::Coord b, Board::Coord c, Board::Coord d) {
        test_in_range(a);
        test_in_range(b);
        test_in_range(c);
        test_in_range(d);
      });
}

TEST(Game, Board) {
  Board b;
  for (std::size_t row = 0; row < Board::kNumRows; ++row) {
    for (std::size_t col = 0; col < Board::kNumCols; ++col) {
      EXPECT_EQ(b.get_value(row, col), 0);

      // Legal values can be 0, 1, 2, 3.
      std::uint8_t value = (row + col) % 4;

      b.set_value(row, col, value);
      EXPECT_EQ(b.get_value(row, col), value);

      value = 3 - value;

      b.set_value(row, col, value);
      EXPECT_EQ(b.get_value(row, col), value);
    }
  }
}

std::size_t num_winners(const Board &board, std::uint8_t p) {
  std::size_t counter = 0;
  Board::combos([&counter, board, p](Board::Coord a, Board::Coord b,
                                     Board::Coord c, Board::Coord d) {
    if (board.get_value(a.first, a.second) == p &&
        board.get_value(b.first, b.second) == p &&
        board.get_value(c.first, c.second) == p &&
        board.get_value(d.first, d.second) == p) {
      ++counter;
    }
  });
  return counter;
}

// Returns the row into which a move at col goes.
// Returns 6 if the column is full.
std::size_t drop(const Board &b, std::size_t col) {
  for (std::size_t row = 0; row < 6; ++row) {
    if (b.get_value(row, col) == 0) {
      return row;
    }
  }
  return 6;
}

// player is whose turn it is (1 or 2).
// There are not yet any winners.
// Returns 1 if player 1 wins,
// 2 if player 0 wins,
// 0 if it is a draw.
std::size_t eval(Board &b, std::uint8_t player) {
  const std::uint8_t other = 3 - player;
  bool has_move = false;
  bool has_draw = false;
  for (std::size_t col = 0; col < 7; ++col) {
    const size_t row = drop(b, col);
    if (row == 6) {
      continue;
    }
    has_move = true;
    b.set_value(row, col, player);
    if (b.IsGameOver() == (player == 1 ? Board::Outcome::kRedWins
                                       : Board::Outcome::kYellowWins)) {
      b.set_value(row, col, 0);
      return player;
    }
    const std::size_t e = eval(b, other);
    if (e == player) {
      b.set_value(row, col, 0);
      return player;
    }
    if (e == 0) {
      has_draw = true;
    }
    b.set_value(row, col, 0);
  }
  if (!has_move || has_draw) {
    return 0;
  }
  return other;
}

TEST(Game, Win) {
  Board b;
  EXPECT_EQ(b.IsGameOver(), Board::Outcome::kContested);
}

class ParseTest : public testing::Test {
 protected:
  static void test(const std::string &image, std::uint8_t player,
                   std::size_t count) {
    const Board board = parse(image);
    EXPECT_EQ(num_winners(board, player), count);
  }
};

TEST_F(ParseTest, Parse1) {
  test(
      R"(
.......
.......
..1....
...1...
....1..
.....1.
)",
      1, 1);
};

TEST_F(ParseTest, Parse2) {
  test(
      R"(
.......
.1.....
..1....
...1...
....1..
.......
)",
      1, 1);
};

TEST_F(ParseTest, Parse3) {
  test(
      R"(
2222...
.......
.......
.......
.......
.......
)",
      2, 1);
};

TEST_F(ParseTest, Parse4) {
  test(
      R"(
...2222
.......
.......
.......
.......
.......
)",
      2, 1);
};

TEST_F(ParseTest, Parse5) {
  test(
      R"(
...2222
.......
2..2...
2...2..
2....2.
2..2222
)",
      2, 4);
};

TEST_F(ParseTest, Parse6) {
  test(
      R"(
2..222.
.2.2.2.
...2...
222.2..
222..2.
2.2.22.
)",
      2, 1);  // 0 0, 1 1, 2 2, 3 3
};

TEST_F(ParseTest, Parse7) {
  test(
      R"(
.......
.......
...2..2
....2.2
.....22
...2222
)",
      2, 3);
};

TEST_F(ParseTest, Parse8) {
  test(
      R"(
.2....2
..2...2
...2..2
....2.2
.....22
.222222
)",
      2, 9);
};

TEST(Game, PushPop) {
  Board b;
  b.push(5);
  b.push(3);
  b.push(1);
  EXPECT_EQ(b.legal_moves().size(), Board::kNumCols);
  const Board save(b);
  b.push(3);
  b.push(6);
  b.push(2);
  b.pop();
  b.pop();
  b.pop();
  EXPECT_TRUE(b == save);
}

TEST(Game, LegaMoves) {
  Board b;
  for (std::size_t i = 0; i < Board::kNumRows; ++i) {
    b.push(1);
    b.push(3);
    b.push(5);
  }

  const std::vector<std::size_t> expected{0, 2, 4, 6};
  EXPECT_EQ(b.legal_moves(), expected);
}

TEST(Game, Eval) {
  const std::vector<std::tuple<std::string, std::uint8_t, std::uint8_t>> data =
      {
          {
              R"(
11..122
21..212
12..121
2121211
1221122
2112211
)",
              1, 1},

          {
              R"(
112.121
2122212
1211122
2122211
1221122
2111211
)",
              1, 0},
      };
  for (const auto &[s, player, ans] : data) {
    Board b = parse(s);
    EXPECT_TRUE(b.IsGameOver() == Board::Outcome::kContested);
    EXPECT_EQ(eval(b, player), ans);
  }
}

class HeurTest : public testing::Test {
 protected:
  static void test(const std::string &image, int expected) {
    Board board = parse(image);
    board.set_favorite(1);
    const int actual = board.heuristic();
    EXPECT_EQ(actual, expected);
    board.set_favorite(2);
    EXPECT_EQ(-actual, board.heuristic());
  }
};

TEST_F(HeurTest, Heur1) {
  test(
      R"(
.......
.......
.......
.......
.......
.......
)",
      0);
}

TEST_F(HeurTest, Heur2) {
  test(
      R"(
.......
.......
.......
.......
.......
...1...
)",
      7);
}

TEST_F(HeurTest, Heur3) {
  test(
      R"(
.......
.......
.......
.......
.......
...12..
)",
      2);
}

TEST_F(HeurTest, Heur4) {
  test(
      R"(
.......
.......
.......
.......
...1...
...12..
)",
      12);
}

TEST_F(HeurTest, Heur5) {
  test(
      R"(
.......
.......
...1...
...1...
..212..
..212..
)",
      1000);
}

TEST(Eval, Empty) {
  Board b;
  // The first move on an empty board should be the center column.
  // This should not be rocket science.
  b.set_favorite(1);
  EXPECT_EQ(b.find_move(/*depth=*/5), 1);  // ??
}

TEST(Eval, ForTheWin) {
  Board b = parse(R"(
.......
.......
.......
..1....
.212...
.212...
)");
  b.set_favorite(1);
  EXPECT_EQ(b.find_move(/*depth=*/5), 2);
}

TEST(Eval, ForTheBlock) {
  Board b = parse(R"(
.......
.......
.......
..2....
.121...
.121...
)");
  b.set_favorite(1);
  EXPECT_EQ(b.find_move(/*depth=*/5), 2);
}

TEST(Eval, GetTheFork) {
  Board b = parse(R"(
.......
.......
.......
...22..
.1121..
12112..
)");
  b.set_favorite(1);
  EXPECT_EQ(b.find_move(/*depth=*/5), 2);
}

TEST(Eval, BlockTheFork) {
  Board b = parse(R"(
.......
.......
.......
...12..
.2212..
21221..
)");
  b.set_favorite(1);
  EXPECT_EQ(b.find_move(/*depth=*/6), 2);
}

TEST(Winner, RedWins) {
  const Board b = parse(R"(
.......
.......
..121..
..212..
.2121..
221112.
)");
  EXPECT_EQ(b.IsGameOver(), Board::Outcome::kYellowWins);
}

TEST(Eval, CheckWin) {
  const Board b = parse(R"(
.1.2.2.
.2.1.2.
21.2.1.
1211222
2122111
1211211
)");
  EXPECT_EQ(b.IsGameOver(), Board::Outcome::kContested);
}

// The outcome of the program playing itself.
// This is a brittle test; it will need to be updated if the
// game AI changes.
TEST(Eval, PlaySelf) {
  Board b;
  b.drop(3);  // First move pre-programmed

  Board::Outcome outcome;
  for (;;) {
    // Yellow move
    b.set_favorite(2);
    b.drop(b.find_move(/*depth=*/6));
    outcome = b.IsGameOver();
    if (outcome != Board::Outcome::kContested) {
      break;
    }

    // Red move
    b.set_favorite(1);
    b.drop(b.find_move(/*depth=*/6));
    outcome = b.IsGameOver();
    if (outcome != Board::Outcome::kContested) {
      break;
    }
  }
  EXPECT_EQ(outcome, Board::Outcome::kYellowWins);
  const Board golden = parse(R"(
212222.
121112.
212221.
121112.
2122112
1211211
)");
  EXPECT_TRUE(b == golden);
}

TEST(ThreeInRow, Empty) {
  const Board b = parse(R"(
.......
.......
.......
.......
.......
.......
)");
  const auto result = b.ThreeInARow(1);
  EXPECT_EQ(result.first, 0);
  EXPECT_EQ(result.second, Board::ThreeKind::kNone);
}

TEST(ThreeInRow, None) {
  const Board b = parse(R"(
.......
.......
.......
2121211
2112212
1212121
)");
  const auto result = b.ThreeInARow(1);
  EXPECT_EQ(result.first, 0);
  EXPECT_EQ(result.second, Board::ThreeKind::kNone);
}

TEST(ThreeInRow, WinOne) {
  const Board b = parse(R"(
.......
.......
.......
....2..
.2..2..
.1.11..
)");
  const auto result = b.ThreeInARow(1);
  EXPECT_EQ(result.first, 2);
  EXPECT_EQ(result.second, Board::ThreeKind::kWin);
}

TEST(ThreeInRow, OneFilled) {
  const Board b = parse(R"(
.......
.......
.......
....2..
.2..2..
.1211..
)");
  const auto result = b.ThreeInARow(1);
  EXPECT_EQ(result.first, 0);
  EXPECT_EQ(result.second, Board::ThreeKind::kNone);
}

TEST(ThreeInRow, WinTwo) {
  const Board b = parse(R"(
.......
.......
....2..
....2..
.2..2..
.1.11..
)");
  const auto result = b.ThreeInARow(2);
  EXPECT_EQ(result.first, 4);
  EXPECT_EQ(result.second, Board::ThreeKind::kWin);
}

TEST(ThreeInRow, BlockTwo) {
  const Board b = parse(R"(
.......
.......
.......
....2..
.2..2..
.1.11..
)");
  const auto result = b.ThreeInARow(2);
  EXPECT_EQ(result.first, 2);
  EXPECT_EQ(result.second, Board::ThreeKind::kBlock);
}

TEST(ThreeInRow, NoSupport) {
  const Board b = parse(R"(
.......
.......
.......
..2....
.212...
.1212..
)");
  const auto result = b.ThreeInARow(2);
  EXPECT_EQ(result.first, 0);
  EXPECT_EQ(result.second, Board::ThreeKind::kNone);
}

TEST(ThreeInRow, LoseTwo) {
  const Board b = parse(R"(
.......
.......
.......
.2..1..
.2..1..
21.112.
)");
  const auto result = b.ThreeInARow(2);
  EXPECT_EQ(result.first, 4);
  EXPECT_EQ(result.second, Board::ThreeKind::kLose);
}

TEST(BruteForce, RedLoses) {
  Board b = parse(R"(
....211
....122
2...211
1..2122
2.11212
1121122
)");
  b.set_whose_turn();
  const auto [result, path] = b.BruteForce(1e7, 1);
  EXPECT_EQ(result, Board::BruteForceResult::kLose);
  const std::vector<std::size_t> expected = {3, 3};
  EXPECT_EQ(path, expected);
}

TEST(BruteForce, YellowWins) {
  Board b = parse(R"(
2......
1.....1
2.....1
1...212
2212121
1112212
)");
  b.set_whose_turn();
  const auto [result, path] = b.BruteForce(1e18, 2);
  EXPECT_EQ(result, Board::BruteForceResult::kWin);
  const std::vector<std::size_t> expected = {6, 5, 4, 5, 5, 4, 4, 3, 3};
  EXPECT_EQ(path, expected);
}

TEST(BruteForce, OneMore) {
  Board b = parse(R"(
...1...
...21..
.2.22.1
.1.12.2
22.2111
1112122
)");
  b.set_whose_turn();
  const auto [result, path] = b.BruteForce(1e18, 1);
  EXPECT_EQ(result, Board::BruteForceResult::kLose);
  const std::vector<std::size_t> expected = {2, 6, 6, 5, 5, 5, 5, 4, 2, 2};
  EXPECT_EQ(path, expected);
}
