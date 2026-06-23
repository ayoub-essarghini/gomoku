// tests/TBoard.cpp - FINAL CLEAN VERSION
#include <iostream>
#include <cassert>
#include <gomoku/Board.hpp>
#include <gomoku/Position.hpp>

#define ASSERT_TRUE(x)  if (!(x)) { std::cout << "FAIL: " #x << std::endl; return false; }
#define ASSERT_FALSE(x) if ((x))  { std::cout << "FAIL: " #x << std::endl; return false; }
#define ASSERT_EQ(a,b)  if (!((a) == (b))) { std::cout << "FAIL: " #a " == " #b << std::endl; return false; }
#define ASSERT_NE(a,b)  if ((a) == (b)) { std::cout << "FAIL: " #a " != " #b << std::endl; return false; }

// ============ BASIC TESTS ============

bool test_Board_initial_empty()
{
    Board board;

    for (int r = 0; r < Board::HEIGHT; r++)
    {
        for (int c = 0; c < Board::WIDTH; c++)
        {
            Position pos{r, c};
            ASSERT_TRUE(board.isEmpty(pos));
            ASSERT_FALSE(board.hasBlue(pos));
            ASSERT_FALSE(board.hasRed(pos));
        }
    }

    std::cout << "PASS: Board initial empty\n";
    return true;
}

bool test_place_blue_stone()
{
    Board board;
    Position p{5, 5};

    board.placeBlue(p);

    ASSERT_TRUE(board.hasBlue(p));
    ASSERT_FALSE(board.hasRed(p));
    ASSERT_FALSE(board.isEmpty(p));

    std::cout << "PASS: place blue stone\n";
    return true;
}

bool test_place_red_stone()
{
    Board board;
    Position p{7, 8};

    board.placeRed(p);

    ASSERT_TRUE(board.hasRed(p));
    ASSERT_FALSE(board.hasBlue(p));
    ASSERT_FALSE(board.isEmpty(p));

    std::cout << "PASS: place red stone\n";
    return true;
}

bool test_no_overwrite_blue()
{
    Board board;
    Position p{5, 5};

    board.placeBlue(p);
    board.placeBlue(p);

    ASSERT_TRUE(board.hasBlue(p));
    ASSERT_EQ(board.countBlueStones(), 1);

    std::cout << "PASS: no double blue stones\n";
    return true;
}

bool test_remove_stone()
{
    Board board;
    Position p{5, 5};

    board.placeBlue(p);
    ASSERT_FALSE(board.isEmpty(p));

    board.removeStone(p);
    ASSERT_TRUE(board.isEmpty(p));

    std::cout << "PASS: remove stone\n";
    return true;
}

bool test_reset()
{
    Board board;

    board.placeBlue({5, 5});
    board.placeRed({7, 8});

    board.reset();

    ASSERT_TRUE(board.isEmpty({5, 5}));
    ASSERT_TRUE(board.isEmpty({7, 8}));
    ASSERT_EQ(board.countBlueStones(), 0);
    ASSERT_EQ(board.countRedStones(), 0);

    std::cout << "PASS: reset\n";
    return true;
}

// ============ COUNTING TESTS ============

bool test_count_blue_stones()
{
    Board board;

    ASSERT_EQ(board.countBlueStones(), 0);

    board.placeBlue({0, 0});
    ASSERT_EQ(board.countBlueStones(), 1);

    board.placeBlue({5, 5});
    ASSERT_EQ(board.countBlueStones(), 2);

    board.placeBlue({18, 18});
    ASSERT_EQ(board.countBlueStones(), 3);

    std::cout << "PASS: count blue stones\n";
    return true;
}

bool test_count_red_stones()
{
    Board board;

    ASSERT_EQ(board.countRedStones(), 0);

    board.placeRed({3, 3});
    ASSERT_EQ(board.countRedStones(), 1);

    board.placeRed({10, 10});
    ASSERT_EQ(board.countRedStones(), 2);

    std::cout << "PASS: count red stones\n";
    return true;
}

bool test_count_mixed()
{
    Board board;

    board.placeBlue({1, 1});
    board.placeBlue({2, 2});
    board.placeRed({3, 3});
    board.placeRed({4, 4});
    board.placeRed({5, 5});

    ASSERT_EQ(board.countBlueStones(), 2);
    ASSERT_EQ(board.countRedStones(), 3);

    std::cout << "PASS: count mixed stones\n";
    return true;
}

// ============ GET STONES TESTS ============

bool test_get_blue_stones()
{
    Board board;

    board.placeBlue({0, 0});
    board.placeBlue({5, 5});
    board.placeBlue({18, 18});

    auto blues = board.getBlueStones();

    ASSERT_EQ(blues.size(), 3);

    std::cout << "PASS: get blue stones\n";
    return true;
}

bool test_get_red_stones()
{
    Board board;

    board.placeRed({1, 1});
    board.placeRed({10, 10});

    auto reds = board.getRedStones();

    ASSERT_EQ(reds.size(), 2);

    std::cout << "PASS: get red stones\n";
    return true;
}

// ============ BOUNDARY TESTS ============

bool test_corners()
{
    Board board;

    board.placeBlue({0, 0});
    ASSERT_TRUE(board.hasBlue({0, 0}));

    board.placeBlue({0, 18});
    ASSERT_TRUE(board.hasBlue({0, 18}));

    board.placeRed({18, 0});
    ASSERT_TRUE(board.hasRed({18, 0}));

    board.placeRed({18, 18});
    ASSERT_TRUE(board.hasRed({18, 18}));

    std::cout << "PASS: corners\n";
    return true;
}

bool test_edge_positions()
{
    Board board;

    board.placeBlue({0, 5});
    board.placeBlue({5, 0});
    board.placeBlue({18, 5});
    board.placeBlue({5, 18});

    ASSERT_EQ(board.countBlueStones(), 4);

    std::cout << "PASS: edge positions\n";
    return true;
}

// ============ STRESS TESTS ============

bool test_fill_partial_board()
{
    Board board;

    board.placeBlue({0, 0});
    board.placeBlue({1, 1});
    board.placeBlue({2, 2});
    board.placeBlue({3, 3});
    board.placeBlue({4, 4});
    
    board.placeRed({5, 5});
    board.placeRed({6, 6});
    board.placeRed({7, 7});
    board.placeRed({8, 8});
    board.placeRed({9, 9});

    ASSERT_EQ(board.countBlueStones(), 5);
    ASSERT_EQ(board.countRedStones(), 5);

    std::cout << "PASS: fill partial board\n";
    return true;
}

bool test_reset_after_fill()
{
    Board board;

    for (int i = 0; i < 20; i++)
    {
        board.placeBlue({i / 19, i % 19});
    }

    ASSERT_NE(board.countBlueStones(), 0);

    board.reset();

    ASSERT_EQ(board.countBlueStones(), 0);
    ASSERT_EQ(board.countRedStones(), 0);

    std::cout << "PASS: reset after fill\n";
    return true;
}

// ============ REMOVE AND REPLACE TESTS ============

bool test_remove_and_replace()
{
    Board board;

    Position p{5, 5};

    board.placeBlue(p);
    ASSERT_TRUE(board.hasBlue(p));

    board.removeStone(p);
    ASSERT_TRUE(board.isEmpty(p));

    board.placeRed(p);
    ASSERT_TRUE(board.hasRed(p));

    std::cout << "PASS: remove and replace\n";
    return true;
}

bool test_multiple_operations()
{
    Board board;

    board.placeBlue({5, 5});
    board.placeBlue({5, 6});
    board.placeRed({6, 5});

    ASSERT_EQ(board.countBlueStones(), 2);
    ASSERT_EQ(board.countRedStones(), 1);

    board.removeStone({5, 5});

    ASSERT_EQ(board.countBlueStones(), 1);
    ASSERT_TRUE(board.isEmpty({5, 5}));
    ASSERT_TRUE(board.hasBlue({5, 6}));

    std::cout << "PASS: multiple operations\n";
    return true;
}

// ============ MAIN TEST RUNNER ============

int main()
{
    std::cout << "=== Board Test Suite ===\n\n";

    int passed = 0;
    int total = 0;

    total++; if (test_Board_initial_empty()) passed++;
    total++; if (test_place_blue_stone()) passed++;
    total++; if (test_place_red_stone()) passed++;
    total++; if (test_no_overwrite_blue()) passed++;
    total++; if (test_remove_stone()) passed++;
    total++; if (test_reset()) passed++;
    total++; if (test_count_blue_stones()) passed++;
    total++; if (test_count_red_stones()) passed++;
    total++; if (test_count_mixed()) passed++;
    total++; if (test_get_blue_stones()) passed++;
    total++; if (test_get_red_stones()) passed++;
    total++; if (test_corners()) passed++;
    total++; if (test_edge_positions()) passed++;
    total++; if (test_fill_partial_board()) passed++;
    total++; if (test_reset_after_fill()) passed++;
    total++; if (test_remove_and_replace()) passed++;
    total++; if (test_multiple_operations()) passed++;

    std::cout << "\n=== RESULTS ===\n";
    std::cout << "PASSED: " << passed << "/" << total << " tests\n";

    if (passed == total) {
        std::cout << "✓ All tests passed!\n";
        return 0;
    } else {
        std::cout << "✗ Some tests failed!\n";
        return 1;
    }
}