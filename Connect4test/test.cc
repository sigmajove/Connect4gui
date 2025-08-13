#include <cstdint>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "../board.h"
#include "../cache.h"
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
  b.CheckPartialWins();
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
  b.CheckPartialWins();
  for (std::size_t row = 0; row < Board::kNumRows; ++row) {
    for (std::size_t col = 0; col < Board::kNumCols; ++col) {
      EXPECT_EQ(b.get_value(row, col), 0);

      // Legal values can be 0, 1, 2.
      // Maybe someday 3.
      std::uint8_t value = (row + col) % 3;

      b.set_value(row, col, value);
      b.CheckPartialWins();
      EXPECT_EQ(b.get_value(row, col), value);

      value = 2 - value;

      b.set_value(row, col, value);
      b.CheckPartialWins();
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
    const Board::Position pos = Board::ParsePosition(image);
    EXPECT_EQ(image[0], '\n');
    EXPECT_EQ(pos.image(), image.substr(1));

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

TEST(LegalMoves, NoMoves) {
  Board b = parse(R"(
1122112
1..1...
2..2...
2..11..
1..122.
2..2122
)");
  std::size_t moves[Board::kNumCols];
  const std::size_t n = b.LegalMoves(moves);
  EXPECT_EQ(n, 0);
};

TEST(LegalMoves, ThreeMoves) {
  Board b = parse(R"(
1..2.12
1..1...
2..2...
2..11..
1..122.
2..2122
)");
  std::size_t moves[Board::kNumCols];
  const std::size_t n = b.LegalMoves(moves);
  EXPECT_EQ(n, 3);
  EXPECT_EQ(moves[0], 1);
  EXPECT_EQ(moves[1], 2);
  EXPECT_EQ(moves[2], 4);
};

TEST(LegalMoves, AllMoves) {
  Board b;
  std::size_t moves[Board::kNumCols];
  const std::size_t n = b.LegalMoves(moves);
  EXPECT_EQ(n, 7);
  EXPECT_EQ(moves[0], 0);
  EXPECT_EQ(moves[1], 1);
  EXPECT_EQ(moves[2], 2);
  EXPECT_EQ(moves[3], 3);
  EXPECT_EQ(moves[4], 4);
  EXPECT_EQ(moves[5], 5);
  EXPECT_EQ(moves[6], 6);
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

Board::BoardMask BuildMask(std::size_t row, std::size_t col) {
  return UINT64_C(1) << (7 * row + col);
}

TEST(ThreeInRow, Empty) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
.......
.......
.......
.......
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 1);
  EXPECT_EQ(result.first, 0);
  EXPECT_EQ(result.second, Board::ThreeKind::kNone);
}

TEST(ThreeInRow, None) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
.......
2121211
2112212
1212121
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 1);
  EXPECT_EQ(result.first, 0);
  EXPECT_EQ(result.second, Board::ThreeKind::kNone);
}

TEST(ThreeInRow, WinOne) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
.......
....2..
.2..2..
.1.11..
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 1);
  EXPECT_EQ(result.first, BuildMask(0, 2));
  EXPECT_EQ(result.second, Board::ThreeKind::kWin);
}

TEST(ThreeInRow, OneFilled) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
.......
....2..
.2..2..
.1211..
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 1);
  EXPECT_EQ(result.first, 0);
  EXPECT_EQ(result.second, Board::ThreeKind::kNone);
}

TEST(ThreeInRow, WinTwo) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
....2..
....2..
.2..2..
.1.11..
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 2);
  EXPECT_EQ(result.first, BuildMask(4, 4));
  EXPECT_EQ(result.second, Board::ThreeKind::kWin);
}

TEST(ThreeInRow, BlockTwo) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
.......
....2..
.2..2..
.1.11..
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 2);
  EXPECT_EQ(result.first, BuildMask(0, 2));
  EXPECT_EQ(result.second, Board::ThreeKind::kBlock);
}

TEST(ThreeInRow, NoSupport) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
.......
..2....
.212...
.1212..
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 2);
  EXPECT_EQ(result.first, 0);
  EXPECT_EQ(result.second, Board::ThreeKind::kNone);
}

TEST(ThreeInRow, LoseTwo) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
.......
.2..1..
.2..1..
21.112.
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 2);
  EXPECT_EQ(result.first, BuildMask(0, 2));
  EXPECT_EQ(result.second, Board::ThreeKind::kLose);
}

TEST(ThreeInRow, Linear) {
  const Board::Position b = Board::ParsePosition(R"(
2...211
1...122
21..211
12.2122
2111212
1121122
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 2);
  EXPECT_EQ(result.first, BuildMask(2, 2));
  EXPECT_EQ(result.second, Board::ThreeKind::kBlock);
}

TEST(ThreeInRow, HorizonalThree) {
  const Board::Position b = Board::ParsePosition(R"(
.......
.......
.......
.......
.111...
2221122
)");
  const auto result = Board::ThreeInARow2(b.red_set, b.yellow_set, 2);
  EXPECT_EQ(result.first, BuildMask(1, 0));
  EXPECT_EQ(result.second, Board::ThreeKind::kLose);
}

TEST(MaskImageTest, Simple) {
  EXPECT_EQ(MaskImage(0), "");
  EXPECT_EQ(MaskImage(BuildMask(3, 2)), "Row 3 Col 2");
  EXPECT_EQ(MaskImage(BuildMask(3, 2) | BuildMask(5, 1)),
            "Row 3 Col 2, Row 5 Col 1");
}

std::pair<Board::Outcome, std::vector<std::size_t>> PlaySelfTest(
    Board::Position &p) {
  std::vector<std::size_t> result;
  for (;;) {
    std::cout << "====\n" << p.image() << "====\n";
    const Board::Outcome outcome = p.IsGameOver();
    if (outcome != Board::Outcome::kContested) {
      std::cout << "Outcome: " << DebugImage(outcome) << "\n";
      return std::make_pair(outcome, result);
    }
    auto [_, move] = Board::BruteForce4(p, 1e18);

    // Chose one of the moves returned (the one with the smallest row).
    const int offset = std::countr_zero(move);
    const Board::BoardMask mask = OneMask(offset);
    const std::size_t me = p.WhoseTurn();
    switch (me) {
      case 1:
        p.red_set |= mask;
        break;
      case 2:
        p.yellow_set |= mask;
        break;
    }
    std::cout << me << " moves in " << MaskImage(mask) << "\n";
    result.push_back(offset % Board::kNumCols);
  }
}

TEST(PlayTest, YellowIn2) {
  Board::Position p = Board::ParsePosition(R"(
....211
....122
2...211
1..2122
2.11212
1121122
)");
  auto [outcome, path] = PlaySelfTest(p);
  EXPECT_EQ(outcome, Board::Outcome::kYellowWins);
  const std::vector<std::size_t> expected = {1, 3, 3, 0, 1, 1,
                                             1, 0, 1, 3, 2, 2};
  EXPECT_EQ(path, expected);
}

TEST(PlayTest, OneMore) {
  Board::Position p = Board::ParsePosition(R"(
...1...
...21..
.2.22.1
.1.12.2
22.2111
1112122
)");
  EXPECT_EQ(p.WhoseTurn(), 1);
  const auto [result, path] = PlaySelfTest(p);
  EXPECT_EQ(result, Board::Outcome::kYellowWins);
  const std::vector<std::size_t> expected = {2, 1, 0, 2, 2, 6, 1, 4,
                                             6, 0, 0, 0, 2, 5, 5, 2};
  EXPECT_EQ(path, expected);
}

TEST(PlayTest, YellowIn6) {
  Board::Position p = Board::ParsePosition(R"(
2......
1.....1
2.....1
1...212
2212121
1112212
)");
  auto [outcome, path] = PlaySelfTest(p);
  EXPECT_EQ(outcome, Board::Outcome::kYellowWins);
  const std::vector<std::size_t> expected = {2, 2, 3, 1, 1};
  EXPECT_EQ(path, expected);
}

TEST(PlayTest, YellowIn10) {
  Board::Position p = Board::ParsePosition(R"(
...1...
...21..
.2.22.1
.1.12.2
22.2111
1112122
)");
  auto [outcome, path] = PlaySelfTest(p);
  EXPECT_EQ(outcome, Board::Outcome::kYellowWins);
  const std::vector<std::size_t> expected = {2, 1, 0, 2, 2, 6, 1, 4,
                                             6, 0, 0, 0, 2, 5, 5, 2};
  EXPECT_EQ(path, expected);
}

TEST(BruteForce, RedLoses) {
  Board::Position p = Board::ParsePosition(R"(
....211
....122
2...211
1..2122
2.11212
1121122
)");
  EXPECT_EQ(p.WhoseTurn(), 1);
  auto [outcome, path] = PlaySelfTest(p);
  EXPECT_EQ(outcome, Board::Outcome::kYellowWins);
  const std::vector<std::size_t> expected = {1, 3, 3, 0, 1, 1,
                                             1, 0, 1, 3, 2, 2};
  EXPECT_EQ(path, expected);
}

TEST(BruteForce, Simple) {
  Board::Position p = Board::ParsePosition(R"(
....211
....122
2...211
1..2122
2.11212
1121122
)");
  EXPECT_EQ(p.WhoseTurn(), 1);
  const auto [result, move] = Board::BruteForce4(p, 1e12);
  EXPECT_EQ(result, BruteForceResult::kLose);
  EXPECT_EQ(MaskImage(move), "Row 1 Col 1, Row 4 Col 0");
}

TEST(BruteForce, YellowWins) {
  Board::Position p = Board::ParsePosition(R"(
2......
1.....1
2.....1
1...212
2212121
1112212
)");
  EXPECT_EQ(p.WhoseTurn(), 2);
  const auto [result, move] = Board::BruteForce4(p, 1e13);
  EXPECT_EQ(DebugImage(result), "Win");
  EXPECT_EQ(MaskImage(move), "Row 2 Col 2");
}

TEST(BruteForce, OneMore4) {
  Board::Position p = Board::ParsePosition(R"(
...1...
...21..
.2.22.1
.1.12.2
22.2111
1112122
)");
  EXPECT_EQ(p.WhoseTurn(), 1);
  const auto [result, move] = Board::BruteForce4(p, 1e18);
  EXPECT_EQ(DebugImage(result), "Lose");
  EXPECT_EQ(MaskImage(move), "Row 1 Col 2");
}

struct CacheData {
  std::uint64_t key1;
  std::uint64_t key2;
  std::size_t value;
};

std::vector<CacheData> cache_data = {
    {2323232, 2323232, 34343}, {2323231, 2323232, 34345},
    {2323232, 2323231, 34341}, {3323232, 2323232, 44343},
    {3323231, 2323232, 44345}, {3323232, 2323231, 44341}};

TEST(Cache, Basic) {
  Cache<std::size_t> cache(2, 20);

  for (std::size_t i = 0; i < cache_data.size(); ++i) {
    {
      const auto [key1, key2, value] = cache_data[i];
      EXPECT_FALSE(cache.Lookup(key1, key2).has_value());
      cache.Insert(key1, key2, value);
    }
    for (std::size_t j = 0; j <= i; j++) {
      const auto [key1, key2, value] = cache_data[j];
      const auto xxx = cache.Lookup(key1, key2);
      EXPECT_TRUE(xxx.has_value());
      EXPECT_EQ(*xxx, value);
    }
    for (std::size_t j = i + 1; j < cache_data.size(); j++) {
      const auto [key1, key2, value] = cache_data[j];
      EXPECT_FALSE(cache.Lookup(key1, key2).has_value());
    }
  }
}

TEST(Cache, BasicLru) {
  Cache<std::size_t> cache(11, 20);
  std::vector<std::pair<std::uint64_t, std::uint64_t>> keys;
  for (std::uint64_t k = 0; k < 10; ++k) {
    cache.Insert(k, 100, k + 1000);
    cache.LruOrder();
    keys.emplace_back(k, 100);
  }
  std::reverse(keys.begin(), keys.end());
  EXPECT_EQ(cache.LruOrder(), keys);
  {
    const auto x = cache.Lookup(5, 100);
    ASSERT_TRUE(x.has_value());
    EXPECT_EQ(*x, 1005);
  }
  const std::vector<std::pair<std::uint64_t, std::uint64_t>> expected = {
      {5, 100}, {9, 100}, {8, 100}, {7, 100}, {6, 100},
      {4, 100}, {3, 100}, {2, 100}, {1, 100}, {0, 100}};
  EXPECT_EQ(cache.LruOrder(), expected);

  {
    // Do it again.
    const auto x = cache.Lookup(5, 100);
    ASSERT_TRUE(x.has_value());
    EXPECT_EQ(*x, 1005);
  }

  // Cache shouldn't change.
  EXPECT_EQ(cache.LruOrder(), expected);

  {
    const auto x = cache.Lookup(0, 100);
    ASSERT_TRUE(x.has_value());
    EXPECT_EQ(*x, 1000);
  }

  const std::vector<std::pair<std::uint64_t, std::uint64_t>> reorder = {
      {0, 100}, {5, 100}, {9, 100}, {8, 100}, {7, 100},
      {6, 100}, {4, 100}, {3, 100}, {2, 100}, {1, 100}};
  EXPECT_EQ(cache.LruOrder(), reorder);
}

TEST(Cache, Dropoff) {
  Cache<std::size_t> cache(11, 9);
  for (std::uint64_t k = 0; k < 10; ++k) {
    cache.Insert(k, 100, k + 1000);
  }
  // Check that (0, 100) has dropped out.
  {
    const std::vector<std::pair<std::uint64_t, std::uint64_t>> expected = {
        {9, 100}, {8, 100}, {7, 100}, {6, 100}, {5, 100},
        {4, 100}, {3, 100}, {2, 100}, {1, 100}};
    EXPECT_EQ(cache.LruOrder(), expected);
    EXPECT_FALSE(cache.Lookup(0, 100).has_value());
  }
}

TEST(Cache, ShuffleAndDrop) {
  Cache<std::size_t> cache(11, 9);
  for (std::uint64_t k = 0; k < 9; ++k) {
    cache.Insert(k, 100, k + 1000);
  }
  {
    const std::vector<std::pair<std::uint64_t, std::uint64_t>> expected = {
        {8, 100}, {7, 100}, {6, 100}, {5, 100}, {4, 100},
        {3, 100}, {2, 100}, {1, 100}, {0, 100}};
    EXPECT_EQ(cache.LruOrder(), expected);
  }
  cache.Lookup(2, 100);
  {
    const std::vector<std::pair<std::uint64_t, std::uint64_t>> expected = {
        {2, 100}, {8, 100}, {7, 100}, {6, 100}, {5, 100},
        {4, 100}, {3, 100}, {1, 100}, {0, 100}};
    EXPECT_EQ(cache.LruOrder(), expected);
  }
  cache.Lookup(0, 100);
  {
    const std::vector<std::pair<std::uint64_t, std::uint64_t>> expected = {
        {0, 100}, {2, 100}, {8, 100}, {7, 100}, {6, 100},
        {5, 100}, {4, 100}, {3, 100}, {1, 100}};
    EXPECT_EQ(cache.LruOrder(), expected);
  }
  cache.Insert(9, 100, 1009);
  {
    const std::vector<std::pair<std::uint64_t, std::uint64_t>> expected = {
        {9, 100}, {0, 100}, {2, 100}, {8, 100}, {7, 100},
        {6, 100}, {5, 100}, {4, 100}, {3, 100}};
    EXPECT_EQ(cache.LruOrder(), expected);
    EXPECT_FALSE(cache.Lookup(1, 100).has_value());
  }
}

TEST(Cache, TableSizes) {
  Cache<std::size_t> a(1, 10);
  EXPECT_EQ(a.hash_shift(), 64);

  Cache<std::size_t> b(2, 10);
  EXPECT_EQ(b.hash_shift(), 63);

  Cache<std::size_t> c(3, 10);
  EXPECT_EQ(c.hash_shift(), 62);

  Cache<std::size_t> d(4, 10);
  EXPECT_EQ(d.hash_shift(), 62);

  Cache<std::size_t> e(5, 10);
  EXPECT_EQ(e.hash_shift(), 61);

  Cache<std::size_t> f(64, 10);
  EXPECT_EQ(f.hash_shift(), 58);

  Cache<std::size_t> g(65, 10);
  EXPECT_EQ(g.hash_shift(), 57);
}
