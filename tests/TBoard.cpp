// tests/TBoard.cpp - UPDATED CLEAN VERSION
#include <iostream>
#include <cassert>
#include <gomoku/Board.hpp>
#include <gomoku/Position.hpp>
#include <gomoku/Stone.hpp>

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
            ASSERT_FALSE(board.hasStone(pos, Stone::BLACK));
            ASSERT_FALSE(board.hasStone(pos, Stone::WHITE));
        }
    }

    std::cout << "PASS: Board initial empty\n";
    return true;
}

bool test_place_black_stone()
{
    Board board;
    Position p{5, 5};

    board.placeStone(p, Stone::BLACK);

    ASSERT_TRUE(board.hasStone(p, Stone::BLACK));
    ASSERT_FALSE(board.hasStone(p, Stone::WHITE));
    ASSERT_FALSE(board.isEmpty(p));

    std::cout << "PASS: place black stone\n";
    return true;
}

bool test_place_white_stone()
{
    Board board;
    Position p{7, 8};

    board.placeStone(p, Stone::WHITE);

    ASSERT_TRUE(board.hasStone(p, Stone::WHITE));
    ASSERT_FALSE(board.hasStone(p, Stone::BLACK));
    ASSERT_FALSE(board.isEmpty(p));

    std::cout << "PASS: place white stone\n";
    return true;
}

bool test_no_overwrite_black()
{
    Board board;
    Position p{5, 5};

    board.placeStone(p, Stone::BLACK);
    board.placeStone(p, Stone::BLACK);

    ASSERT_TRUE(board.hasStone(p, Stone::BLACK));
    ASSERT_EQ(board.countStones(Stone::BLACK), 1);

    std::cout << "PASS: no double black stones\n";
    return true;
}

bool test_remove_stone()
{
    Board board;
    Position p{5, 5};

    board.placeStone(p, Stone::BLACK);
    ASSERT_FALSE(board.isEmpty(p));

    board.removeStone(p);
    ASSERT_TRUE(board.isEmpty(p));

    std::cout << "PASS: remove stone\n";
    return true;
}

bool test_reset()
{
    Board board;

    board.placeStone({5, 5}, Stone::BLACK);
    board.placeStone({7, 8}, Stone::WHITE);

    board.reset();

    ASSERT_TRUE(board.isEmpty({5, 5}));
    ASSERT_TRUE(board.isEmpty({7, 8}));
    ASSERT_EQ(board.countStones(Stone::BLACK), 0);
    ASSERT_EQ(board.countStones(Stone::WHITE), 0);

    std::cout << "PASS: reset\n";
    return true;
}

// ============ COUNTING TESTS ============

bool test_count_black_stones()
{
    Board board;

    ASSERT_EQ(board.countStones(Stone::BLACK), 0);

    board.placeStone({0, 0}, Stone::BLACK);
    ASSERT_EQ(board.countStones(Stone::BLACK), 1);

    board.placeStone({5, 5}, Stone::BLACK);
    ASSERT_EQ(board.countStones(Stone::BLACK), 2);

    board.placeStone({18, 18}, Stone::BLACK);
    ASSERT_EQ(board.countStones(Stone::BLACK), 3);

    std::cout << "PASS: count black stones\n";
    return true;
}

bool test_count_white_stones()
{
    Board board;

    ASSERT_EQ(board.countStones(Stone::WHITE), 0);

    board.placeStone({3, 3}, Stone::WHITE);
    ASSERT_EQ(board.countStones(Stone::WHITE), 1);

    board.placeStone({10, 10}, Stone::WHITE);
    ASSERT_EQ(board.countStones(Stone::WHITE), 2);

    std::cout << "PASS: count white stones\n";
    return true;
}

bool test_count_mixed()
{
    Board board;

    board.placeStone({1, 1}, Stone::BLACK);
    board.placeStone({2, 2}, Stone::BLACK);
    board.placeStone({3, 3}, Stone::WHITE);
    board.placeStone({4, 4}, Stone::WHITE);
    board.placeStone({5, 5}, Stone::WHITE);

    ASSERT_EQ(board.countStones(Stone::BLACK), 2);
    ASSERT_EQ(board.countStones(Stone::WHITE), 3);

    std::cout << "PASS: count mixed stones\n";
    return true;
}

// ============ GET STONES TESTS ============

bool test_get_black_stones()
{
    Board board;

    board.placeStone({0, 0}, Stone::BLACK);
    board.placeStone({5, 5}, Stone::BLACK);
    board.placeStone({18, 18}, Stone::BLACK);

    auto blacks = board.getStones(Stone::BLACK);

    ASSERT_EQ(blacks.size(), 3);

    std::cout << "PASS: get black stones\n";
    return true;
}

bool test_get_white_stones()
{
    Board board;

    board.placeStone({1, 1}, Stone::WHITE);
    board.placeStone({10, 10}, Stone::WHITE);

    auto whites = board.getStones(Stone::WHITE);

    ASSERT_EQ(whites.size(), 2);

    std::cout << "PASS: get white stones\n";
    return true;
}

// ============ BOUNDARY TESTS ============

bool test_corners()
{
    Board board;

    board.placeStone({0, 0}, Stone::BLACK);
    ASSERT_TRUE(board.hasStone({0, 0}, Stone::BLACK));

    board.placeStone({0, 18}, Stone::BLACK);
    ASSERT_TRUE(board.hasStone({0, 18}, Stone::BLACK));

    board.placeStone({18, 0}, Stone::WHITE);
    ASSERT_TRUE(board.hasStone({18, 0}, Stone::WHITE));

    board.placeStone({18, 18}, Stone::WHITE);
    ASSERT_TRUE(board.hasStone({18, 18}, Stone::WHITE));

    std::cout << "PASS: corners\n";
    return true;
}

bool test_edge_positions()
{
    Board board;

    board.placeStone({0, 5}, Stone::BLACK);
    board.placeStone({5, 0}, Stone::BLACK);
    board.placeStone({18, 5}, Stone::BLACK);
    board.placeStone({5, 18}, Stone::BLACK);

    ASSERT_EQ(board.countStones(Stone::BLACK), 4);

    std::cout << "PASS: edge positions\n";
    return true;
}

// ============ STRESS TESTS ============

bool test_fill_partial_board()
{
    Board board;

    board.placeStone({0, 0}, Stone::BLACK);
    board.placeStone({1, 1}, Stone::BLACK);
    board.placeStone({2, 2}, Stone::BLACK);
    board.placeStone({3, 3}, Stone::BLACK);
    board.placeStone({4, 4}, Stone::BLACK);
    
    board.placeStone({5, 5}, Stone::WHITE);
    board.placeStone({6, 6}, Stone::WHITE);
    board.placeStone({7, 7}, Stone::WHITE);
    board.placeStone({8, 8}, Stone::WHITE);
    board.placeStone({9, 9}, Stone::WHITE);

    ASSERT_EQ(board.countStones(Stone::BLACK), 5);
    ASSERT_EQ(board.countStones(Stone::WHITE), 5);

    std::cout << "PASS: fill partial board\n";
    return true;
}

bool test_reset_after_fill()
{
    Board board;

    for (int i = 0; i < 20; i++)
    {
        board.placeStone({i / 19, i % 19}, Stone::BLACK);
    }

    ASSERT_NE(board.countStones(Stone::BLACK), 0);

    board.reset();

    ASSERT_EQ(board.countStones(Stone::BLACK), 0);
    ASSERT_EQ(board.countStones(Stone::WHITE), 0);

    std::cout << "PASS: reset after fill\n";
    return true;
}

// ============ REMOVE AND REPLACE TESTS ============

bool test_remove_and_replace()
{
    Board board;
    Position p{5, 5};

    board.placeStone(p, Stone::BLACK);
    ASSERT_TRUE(board.hasStone(p, Stone::BLACK));

    board.removeStone(p);
    ASSERT_TRUE(board.isEmpty(p));

    board.placeStone(p, Stone::WHITE);
    ASSERT_TRUE(board.hasStone(p, Stone::WHITE));

    std::cout << "PASS: remove and replace\n";
    return true;
}

bool test_multiple_operations()
{
    Board board;

    board.placeStone({5, 5}, Stone::BLACK);
    board.placeStone({5, 6}, Stone::BLACK);
    board.placeStone({6, 5}, Stone::WHITE);

    ASSERT_EQ(board.countStones(Stone::BLACK), 2);
    ASSERT_EQ(board.countStones(Stone::WHITE), 1);

    board.removeStone({5, 5});

    ASSERT_EQ(board.countStones(Stone::BLACK), 1);
    ASSERT_TRUE(board.isEmpty({5, 5}));
    ASSERT_TRUE(board.hasStone({5, 6}, Stone::BLACK));

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
    total++; if (test_place_black_stone()) passed++;
    total++; if (test_place_white_stone()) passed++;
    total++; if (test_no_overwrite_black()) passed++;
    total++; if (test_remove_stone()) passed++;
    total++; if (test_reset()) passed++;
    total++; if (test_count_black_stones()) passed++;
    total++; if (test_count_white_stones()) passed++;
    total++; if (test_count_mixed()) passed++;
    total++; if (test_get_black_stones()) passed++;
    total++; if (test_get_white_stones()) passed++;
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