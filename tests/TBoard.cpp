#include <iostream>
#include <cassert>

#include <gomoku/Board.hpp>
#include <gomoku/Position.hpp>
#include <gomoku/Stone.hpp>

#define ASSERT_TRUE(x)  if (!(x)) { std::cout << "FAIL: " #x << std::endl; return false; }
#define ASSERT_FALSE(x) if ((x))  { std::cout << "FAIL: " #x << std::endl; return false; }
#define ASSERT_EQ(a,b)  if (!((a) == (b))) { std::cout << "FAIL: " #a " == " #b << std::endl; return false; }

bool test_board_initial_empty()
{
    Board board;

    for (int r = 0; r < Board::HEIGHT; r++)
    {
        for (int c = 0; c < Board::WIDTH; c++)
        {
            if (board.getStone({r, c}) != Stone::EMPTY)
            {
                std::cout << "FAIL: board not empty at start\n";
                return false;
            }
        }
    }

    std::cout << "PASS: board initial empty\n";
    return true;
}

bool test_place_stone()
{
    Board board;

    Position p{5, 5};

    ASSERT_TRUE(board.placeStone(p, Stone::BLACK));
    ASSERT_EQ(board.getStone(p), Stone::BLACK);

    std::cout << "PASS: place stone\n";
    return true;
}

bool test_no_overwrite()
{
    Board board;

    Position p{5, 5};

    ASSERT_TRUE(board.placeStone(p, Stone::BLACK));
    ASSERT_FALSE(board.placeStone(p, Stone::WHITE));

    ASSERT_EQ(board.getStone(p), Stone::BLACK);

    std::cout << "PASS: no overwrite\n";
    return true;
}

bool test_is_inside()
{
    Board board;

    ASSERT_TRUE(board.isInside({0, 0}));
    ASSERT_TRUE(board.isInside({18, 18}));
    ASSERT_FALSE(board.isInside({19, 19}));
    ASSERT_FALSE(board.isInside({-1, 0}));

    std::cout << "PASS: isInside\n";
    return true;
}

bool test_reset()
{
    Board board;

    board.placeStone({5, 5}, Stone::BLACK);
    board.reset();

    ASSERT_EQ(board.getStone({5, 5}), Stone::EMPTY);

    std::cout << "PASS: reset\n";
    return true;
}

int main()
{
    std::cout << "Running Board tests...\n\n";

    int passed = 0;
    int total = 5;

    if (test_board_initial_empty()) passed++;
    if (test_place_stone()) passed++;
    if (test_no_overwrite()) passed++;
    if (test_is_inside()) passed++;
    if (test_reset()) passed++;

    std::cout << "\nRESULT: " << passed << "/" << total << " tests passed\n";

    return (passed == total) ? 0 : 1;
}