// test_renju_foul.cpp
//
// Standalone harness for RuleChecker::isRenjuFoul. No test framework
// dependency (gtest/catch2 etc.) — just asserts + printed PASS/FAIL so it
// can be dropped into any build without wiring up a new test target.
//
// Build (adjust include/lib paths to your project):
//   g++ -std=c++17 test_renju_foul.cpp src/Rules.cpp src/Board.cpp -Iinclude -o test_renju_foul
//   ./test_renju_foul
//
#include <gomoku/Rules.hpp>
#include <gomoku/Board.hpp>
#include <gomoku/Minmax.hpp>
#include <cmath>
#include <iostream>
#include <string>

namespace {

int g_failures = 0;
int g_total = 0;

void check(bool condition, const std::string& testName, const std::string& detail = "") {
    ++g_total;
    if (condition) {
        std::cout << "[PASS] " << testName << "\n";
    } else {
        ++g_failures;
        std::cout << "[FAIL] " << testName;
        if (!detail.empty()) std::cout << " -- " << detail;
        std::cout << "\n";
    }
}

// Small helper: place several Black stones without triggering foul checks,
// so we can set up a board position before testing the *actual* move.
void placeBlacks(Board& board, std::initializer_list<Position> positions) {
    for (const auto& p : positions) {
        board.placeStone(p, Stone::BLACK);
    }
}

void placeWhites(Board& board, std::initializer_list<Position> positions) {
    for (const auto& p : positions) {
        board.placeStone(p, Stone::WHITE);
    }
}

} // namespace

int main() {
    // -------------------------------------------------------------------
    // Test 1: Classic double-three (the exact shape from the screenshot).
    //
    //         col: 11 12 13
    //   row 7:      .  B  .
    //   row 8:      B  ?  B      <- ? is the move under test
    //   row 9:      .  B  .
    //
    // Pre-existing Black stones form the four flanks of a plus shape.
    // Playing (8,12) simultaneously completes:
    //   - horizontal open three at row 8: cols 11-13
    //   - vertical   open three at col 12: rows 7-9
    // This must be forbidden.
    // -------------------------------------------------------------------
    {
        Board board;
        board.reset();
        placeBlacks(board, {
            {7, 12},   // vertical top flank
            {9, 12},   // vertical bottom flank
            {8, 11},   // horizontal left flank
            {8, 13},   // horizontal right flank
        });

        bool foul = RuleChecker::isRenjuFoul(board, Position{8, 12});
        check(foul, "Test 1: classic double-three is a foul",
              "expected true, got false — double-three not detected");
    }

    // -------------------------------------------------------------------
    // Test 2: Only ONE open three is created — must NOT be a foul.
    // Same as Test 1 but without the vertical flanks.
    // -------------------------------------------------------------------
    {
        Board board;
        board.reset();
        placeBlacks(board, {
            {8, 11},
            {8, 13},
        });

        bool foul = RuleChecker::isRenjuFoul(board, Position{8, 12});
        check(!foul, "Test 2: single open three is NOT a foul",
              "expected false, got true — false positive on a single three");
    }

    // -------------------------------------------------------------------
    // Test 3: Double-three where ONE axis is a BROKEN three.
    //
    // Move under test: (8, 11)
    //
    // Horizontal axis (row 8), existing stones at col 10 and col 13:
    //   col:   9 10 11 12 13 14
    //   before: .  B  ?  .  B  .
    //   after playing (8,11): . B B . B .  -> NOT contiguous, so this
    //   axis alone is not the broken-three; it's a plain flank setup.
    //   (Kept simple/non-contributing here so the vertical axis below is
    //   the one actually under test for "broken".)
    //
    // Vertical axis (col 11), existing stones at row 6 and row 9, with the
    // move filling row 8 — leaving row 7 as the internal gap:
    //   row:    6  7  8  9
    //   before: B  .  ?  B
    //   after playing (8,11): B . B B  -> this is a BROKEN three
    //   (".B.bitboard." pattern once padded with the surrounding empties),
    //   which must still register as a live three via simulate-and-check.
    //
    // Horizontal axis (row 8) additionally gets a clean flanking pair so
    // that TWO axes are live simultaneously, making the move a foul:
    //   existing stones at (8, 9) and (8, 13); move at (8, 11):
    //   col:    9 10 11 12 13
    //   before: B  .  ?  .  B
    //   after:  B  .  B  .  B   -> not contiguous either; instead use
    //   adjacent flanks so it collapses into a real three:
    // -------------------------------------------------------------------
    {
        Board board;
        board.reset();
        placeBlacks(board, {
            // Vertical broken three through (8,11): rows 6 and 9, move
            // fills row 8, leaving row 7 as the internal gap.
            {6, 11},
            {9, 11},
            // Horizontal plain three through (8,11): flanks directly
            // adjacent to the move so it collapses into ".bitboardB." (live).
            {8, 10},
            {8, 12},
        });

        bool foul = RuleChecker::isRenjuFoul(board, Position{8, 11});
        check(foul, "Test 3: double-three with a broken three on one axis is a foul",
              "expected true, got false — broken-three axis not detected");
    }

    // -------------------------------------------------------------------
    // Test 4: A three blocked on one end is a FOUR-threat but NOT a live
    // three (can't become an open/unstoppable four), so it must not count
    // toward the double-three foul.
    //
    // Move under test: (8, 12)
    //   row 8, cols 9..13: W  B  B  [?]  .
    //   after playing (8,12): W B B B .  == "WbitboardB."
    // This is a real threat (hasFourThreat should catch it if it were 4
    // stones — here it's still only a three), but crucially it can be
    // fully shut down by White playing the single remaining empty cell,
    // so isLiveThree must return false for this axis, and this move alone
    // must not be flagged as a foul.
    // -------------------------------------------------------------------
    {
        Board board;
        board.reset();
        placeWhites(board, { {8, 9} });
        placeBlacks(board, { {8, 10}, {8, 11} });

        bool foul = RuleChecker::isRenjuFoul(board, Position{8, 12});
        check(!foul, "Test 4: a three blocked on one side is NOT live (not a foul alone)",
              "expected false, got true — blocked three incorrectly counted as live");
    }

    // -------------------------------------------------------------------
    // Test 5: Double-four must be a foul.
    // Horizontal: .bitboardbitboard. type four via existing B B _ B with move filling.
    // Vertical:   same idea on the other axis.
    // -------------------------------------------------------------------
    {
        Board board;
        board.reset();
        placeBlacks(board, {
            // Horizontal four-threat through (8,12): B B [?] B .
            {8, 10}, {8, 11}, {8, 13},
            // Vertical four-threat through (8,12): B B [?] B .
            {6, 12}, {7, 12}, {9, 12},
        });

        bool foul = RuleChecker::isRenjuFoul(board, Position{8, 12});
        check(foul, "Test 5: double-four is a foul",
              "expected true, got false — double-four not detected");
    }

    // -------------------------------------------------------------------
    // Test 6: Overline (6+) must always be a foul, even without any
    // three/four logic being relevant.
    // -------------------------------------------------------------------
    {
        Board board;
        board.reset();
        placeBlacks(board, { {8, 8}, {8, 9}, {8, 10}, {8, 11}, {8, 12} });

        bool foul = RuleChecker::isRenjuFoul(board, Position{8, 13});
        check(foul, "Test 6: overline (6 in a row) is a foul",
              "expected true, got false — overline not detected");
    }

    // -------------------------------------------------------------------
    // Test 7: An exact five-in-a-row is a WIN, never a foul — even if the
    // same move would otherwise look like it creates extra threats.
    // -------------------------------------------------------------------
    {
        Board board;
        board.reset();
        placeBlacks(board, { {8, 8}, {8, 9}, {8, 10}, {8, 11} });

        bool foul = RuleChecker::isRenjuFoul(board, Position{8, 12});
        check(!foul, "Test 7: exact five-in-a-row is NOT a foul",
              "expected false, got true — five wrongly flagged as a foul");
    }

    // -------------------------------------------------------------------
    // Minimax AI tests
    // -------------------------------------------------------------------
    {
        Board board;
        MinimaxAI ai(2);

        Position move = ai.findBestMove(
            board, Stone::BLACK, RuleVariant::STANDARD, 0, 0);

        check(move.x == 9 && move.y == 9,
              "AI Test 1: empty board starts in the center");
    }

    {
        Board board;
        MinimaxAI ai(2);
        placeWhites(board, { {9, 5}, {9, 6}, {9, 7}, {9, 8} });
        placeBlacks(board, { {8, 5}, {8, 7} });

        Position move = ai.findBestMove(
            board, Stone::WHITE, RuleVariant::STANDARD, 0, 0);

        bool completesFive =
            (move.x == 9 && move.y == 4) ||
            (move.x == 9 && move.y == 9);

        check(completesFive,
              "AI Test 2: AI takes an immediate winning move");
    }

    {
        Board board;
        MinimaxAI ai(2);
        placeBlacks(board, { {9, 5}, {9, 6}, {9, 7}, {9, 8} });
        placeWhites(board, { {8, 5}, {8, 7} });

        Position move = ai.findBestMove(
            board, Stone::WHITE, RuleVariant::STANDARD, 0, 0);

        bool blocksFive =
            (move.x == 9 && move.y == 4) ||
            (move.x == 9 && move.y == 9);

        check(blocksFive,
              "AI Test 3: AI blocks an immediate loss");
    }

    {
        Board board;
        MinimaxAI ai(2);
        placeWhites(board, { {9, 5} });
        placeBlacks(board, { {9, 6}, {9, 7} });

        int blackBefore = board.countStones(Stone::BLACK);
        int whiteBefore = board.countStones(Stone::WHITE);

        Position move = ai.findBestMove(
            board, Stone::WHITE, RuleVariant::STANDARD, 0, 4);

        check(move.x == 9 && move.y == 8,
              "AI Test 4: AI takes a capture victory");

        check(board.countStones(Stone::BLACK) == blackBefore &&
                  board.countStones(Stone::WHITE) == whiteBefore &&
                  board.isEmpty(move),
              "AI Test 5: search does not mutate the real board");
    }

    {
        Board board;
        MinimaxAI ai(1);
        placeBlacks(board, { {9, 9} });
        placeWhites(board, { {0, 0} });

        Position move = ai.findBestMove(
            board, Stone::BLACK, RuleVariant::PRO, 0, 0);

        bool outsideCenterFive =
            std::abs(move.x - 9) > 2 ||
            std::abs(move.y - 9) > 2;

        check(move.x >= 0 && move.y >= 0 && outsideCenterFive,
              "AI Test 6: AI respects the Pro third-move restriction");
    }

    {
        Board board;
        MinimaxAI ai(1);
        placeBlacks(board, {
            {7, 12},
            {9, 12},
            {8, 11},
            {8, 13},
        });

        Position move = ai.findBestMove(
            board, Stone::BLACK, RuleVariant::RENJU, 0, 0);

        bool avoidedDoubleThree =
            move.x != 8 || move.y != 12;

        check(move.x >= 0 && move.y >= 0 && avoidedDoubleThree,
              "AI Test 7: Black AI avoids a Renju double-three foul");
    }

    std::cout << "\n" << (g_total - g_failures) << "/" << g_total << " tests passed.\n";
    return g_failures == 0 ? 0 : 1;
}
