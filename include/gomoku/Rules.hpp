#pragma once
#include <gomoku/Board.hpp>
#include <array>
#include <cstdint>
#include <string>
enum class RuleVariant : uint8_t {
    STANDARD,
    PRO,
    SWAP,
    SWAP2,
    RENJU
};
enum class Swap2Phase : uint8_t {
    PLACE_INITIAL_3,
    PLAYER2_CHOICE,
    PLACE_EXTRA_2, 
    PLAYER1_CHOICE,
    FINISHED
};
struct Bitboard {
    std::array<uint32_t, 19> rows{};
};

struct WinLine {
    bool found = false;
    std::vector<Position> cells;
};


class RuleChecker {
    public:
        static Bitboard extractBitboard(const Board& board, Stone color);
        static bool hasFiveInARow(const Bitboard& bitBoard);
        static bool hasOverline(const Bitboard& bitBoard);
        static bool isValidProMove(int moveCount, const Position& pos);
        static bool isRenjuFoul(const Board& board, const Position& pos);
        static int applyCaptures(Board& board, const Position& pos, Stone color);
        static WinLine findWinningLine(const Bitboard& board);
        static bool isFiveCapturable(const Board& board,const WinLine& line, Stone color);

};
class OpeningManager {
    public:
        explicit OpeningManager(RuleVariant variant = RuleVariant::STANDARD);
        void reset(RuleVariant newVariant);
        
        RuleVariant getVariant() const { return variant_; }
        Swap2Phase getSwap2Phase() const { return swapPhase_; }
        bool isOpeningFinished() const;
        bool processSwap2Choice(int choiceOption, Stone& currentTurn);

        void notifyStonePlaced();

    private:
        RuleVariant variant_;
        Swap2Phase swapPhase_;
        int openingStonesPlaced_ = 0;
};