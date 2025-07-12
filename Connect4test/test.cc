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
  for (std::size_t row = 0; row < 6; ++row) {
    for (std::size_t col = 0; col < 7; ++col) {
      EXPECT_EQ(b.get_value(row, col), 0);
      b.set_value(row, col, (row + col) % 2);
      EXPECT_EQ(b.get_value(row, col), (row + col) % 2);
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
#if 0
      std::cout << std::format("{} {}, {} {}, {} {}, {} {}\n", a.first,
                               a.second, b.first, b.second, c.first, c.second,
                               d.first, d.second);
#endif
      ++counter;
    }
  });
  return counter;
}

bool has_winner(const Board &b, std::uint8_t p) {
  for (std::size_t row = 0; row < 6; ++row) {
    for (std::size_t col = 0; col <= 3; ++col) {
      if (b.get_value(row, col) == p && b.get_value(row, col + 1) == p &&
          b.get_value(row, col + 2) == p && b.get_value(row, col + 3) == p) {
        std::cout << "vertical " << row << ", " << col << "\n";
        return true;
      }
    }
  }
  for (std::size_t row = 0; row <= 2; ++row) {
    for (std::size_t col = 0; col < 7; ++col) {
      if (b.get_value(row, col) == p && b.get_value(row + 1, col) == p &&
          b.get_value(row + 2, col) == p && b.get_value(row + 3, col) == p) {
        std::cout << "horizontal " << row << ", " << col << "\n";
        return true;
      }
    }

    for (std::size_t row = 0; row <= 2; ++row) {
      for (std::size_t col = 0; col <= 3; col++) {
        if (b.get_value(row, col) == p && b.get_value(row + 1, col + 1) == p &&
            b.get_value(row + 2, col + 2) == p &&
            b.get_value(row + 3, col + 3) == p) {
          std::cout << "diag 1 " << row << ", " << col << "\n";
          return true;
        }
      }
    }

    for (std::size_t row = 0; row <= 2; ++row) {
      for (std::size_t col = 3; col < 7; col++) {
        if (b.get_value(row, col) == p && b.get_value(row + 1, col - 1) == p &&
            b.get_value(row + 2, col - 2) == p &&
            b.get_value(row + 3, col - 3) == p) {
          std::cout << "diag 2 " << row << ", " << col << "\n";
          return true;
        }
      }
    }
  }
  return false;
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
    if (has_winner(b, player)) {
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
  EXPECT_FALSE(has_winner(b, 1));
  EXPECT_FALSE(has_winner(b, 2));
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
    std::cout << s << "\n";
    Board b = parse(s);
    EXPECT_FALSE(has_winner(b, 1));
    EXPECT_FALSE(has_winner(b, 2));
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
  EXPECT_EQ(b.find_move(/*depth=*/5), 2);
}
