#include <gomoku/Rules.hpp>
#include <cmath>

Bitboard RuleChecker::extractBitboard(const Board& board, Stone color) {
    Bitboard Bitboard;
    for (int r = 0; r < Board::HEIGHT; ++r) {
        for (int c = 0; c < Board::WIDTH; ++c) {
            if (board.hasStone({r, c}, color)) {
                Bitboard.rows[r] |= (1U << c);
            }
        }
    }
    return Bitboard;
}

bool RuleChecker::hasFiveInARow(const Bitboard& Bitboard) {
    for (int r = 0; r < 19; ++r) {
        uint32_t row = Bitboard.rows[r];
        if ((row & (row >> 1) & (row >> 2) & (row >> 3) & (row >> 4)) != 0) {
            return true;
        }
    }

    for (int r = 0; r <= 14; ++r) {
        uint32_t v = Bitboard.rows[r] & Bitboard.rows[r + 1] & Bitboard.rows[r + 2] & Bitboard.rows[r + 3] & Bitboard.rows[r + 4];
        if (v != 0) return true;
    }

    for (int r = 0; r <= 14; ++r) {
        uint32_t d1 = Bitboard.rows[r] & (Bitboard.rows[r + 1] >> 1) & (Bitboard.rows[r + 2] >> 2) 
                    & (Bitboard.rows[r + 3] >> 3) & (Bitboard.rows[r + 4] >> 4);
        if (d1 != 0) return true;
    }

    for (int r = 0; r <= 14; ++r) {
        uint32_t d2 = Bitboard.rows[r] & (Bitboard.rows[r + 1] << 1) & (Bitboard.rows[r + 2] << 2) 
                    & (Bitboard.rows[r + 3] << 3) & (Bitboard.rows[r + 4] << 4);
        if (d2 != 0) return true;
    }

    return false;
}

bool RuleChecker::hasOverline(const Bitboard& Bitboard) {
    for (int r = 0; r < 19; ++r) {
        uint32_t row = Bitboard.rows[r];
        if ((row & (row >> 1) & (row >> 2) & (row >> 3) & (row >> 4) & (row >> 5)) != 0) {
            return true;
        }
    }

    for (int r = 0; r <= 13; ++r) {
        uint32_t v = Bitboard.rows[r] & Bitboard.rows[r + 1] & Bitboard.rows[r + 2] 
                   & Bitboard.rows[r + 3] & Bitboard.rows[r + 4] & Bitboard.rows[r + 5];
        if (v != 0) return true;
    }

    for (int r = 0; r <= 13; ++r) {
        uint32_t d1 = Bitboard.rows[r] & (Bitboard.rows[r + 1] >> 1) & (Bitboard.rows[r + 2] >> 2) 
                    & (Bitboard.rows[r + 3] >> 3) & (Bitboard.rows[r + 4] >> 4) & (Bitboard.rows[r + 5] >> 5);
        if (d1 != 0) return true;
    }

    for (int r = 0; r <= 13; ++r) {
        uint32_t d2 = Bitboard.rows[r] & (Bitboard.rows[r + 1] << 1) & (Bitboard.rows[r + 2] << 2) 
                    & (Bitboard.rows[r + 3] << 3) & (Bitboard.rows[r + 4] << 4) & (Bitboard.rows[r + 5] << 5);
        if (d2 != 0) return true;
    }

    return false;
}

bool RuleChecker::isValidProMove(int moveCount, const Position& pos) {
    if (moveCount == 1) {
        return pos.x == 9 && pos.y == 9;
    }
    if (moveCount == 3) {
        bool insideCenter5x5 = (std::abs(pos.x - 9) <= 2) && (std::abs(pos.y - 9) <= 2);
        return !insideCenter5x5;
    }
    return true;
}

bool RuleChecker::isRenjuFoul(const Board& board, const Position& pos) {
    Board tempBoard = board;
    tempBoard.placeStone(pos, Stone::BLACK);

    Bitboard blackBitboard = extractBitboard(tempBoard, Stone::BLACK);

    if (hasOverline(blackBitboard)) {
        return true;
    }

    return false;
}


OpeningManager::OpeningManager(RuleVariant variant)
    : variant_(variant), swapPhase_(Swap2Phase::PLACE_INITIAL_3) {}

void OpeningManager::reset(RuleVariant newVariant) {
    variant_ = newVariant;
    swapPhase_ = (variant_ == RuleVariant::SWAP2) ? Swap2Phase::PLACE_INITIAL_3 : Swap2Phase::FINISHED;
}

bool OpeningManager::isOpeningFinished() const {
    if (variant_ != RuleVariant::SWAP2) return true;
    return swapPhase_ == Swap2Phase::FINISHED;
}

bool OpeningManager::processSwap2Choice(int choiceOption, Stone& currentTurn) {
    if (variant_ != RuleVariant::SWAP2) return false;

    if (swapPhase_ == Swap2Phase::PLAYER2_CHOICE) {
        if (choiceOption == 1) { 
            swapPhase_ = Swap2Phase::FINISHED;
            currentTurn = Stone::WHITE;
        } else if (choiceOption == 2) { 
            swapPhase_ = Swap2Phase::FINISHED;
            currentTurn = Stone::WHITE;
        } else if (choiceOption == 3) { 
           
            swapPhase_ = Swap2Phase::PLACE_EXTRA_2;
            currentTurn = Stone::BLACK;
        }
        return true;
    } 
    
    if (swapPhase_ == Swap2Phase::PLAYER1_CHOICE) {
        swapPhase_ = Swap2Phase::FINISHED;
        currentTurn = (choiceOption == 1) ? Stone::BLACK : Stone::WHITE;
        return true;
    }

    return false;
}