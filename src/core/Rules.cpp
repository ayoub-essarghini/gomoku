#include <gomoku/Rules.hpp>
#include <cmath>
#include <string>

Bitboard RuleChecker::extractBitboard(const Board& board, Stone color) {
    Bitboard bb;
    for (int r = 0; r < Board::HEIGHT; ++r) {
        for (int c = 0; c < Board::WIDTH; ++c) {
            if (board.hasStone({r, c}, color)) {
                bb.rows[r] |= (1U << c);
            }
        }
    }
    return bb;
}

bool RuleChecker::hasFiveInARow(const Bitboard& bb) {
    for (int r = 0; r < 19; ++r) {
        uint32_t row = bb.rows[r];
        if ((row & (row >> 1) & (row >> 2) & (row >> 3) & (row >> 4)) != 0) {
            return true;
        }
    }

    for (int r = 0; r <= 14; ++r) {
        uint32_t v = bb.rows[r] & bb.rows[r + 1] & bb.rows[r + 2] & bb.rows[r + 3] & bb.rows[r + 4];
        if (v != 0) return true;
    }

    for (int r = 0; r <= 14; ++r) {
        uint32_t d1 = bb.rows[r] & (bb.rows[r + 1] >> 1) & (bb.rows[r + 2] >> 2)
                    & (bb.rows[r + 3] >> 3) & (bb.rows[r + 4] >> 4);
        if (d1 != 0) return true;
    }

    for (int r = 0; r <= 14; ++r) {
        uint32_t d2 = bb.rows[r] & (bb.rows[r + 1] << 1) & (bb.rows[r + 2] << 2)
                    & (bb.rows[r + 3] << 3) & (bb.rows[r + 4] << 4);
        if (d2 != 0) return true;
    }

    return false;
}

bool RuleChecker::hasOverline(const Bitboard& bb) {
    for (int r = 0; r < 19; ++r) {
        uint32_t row = bb.rows[r];
        if ((row & (row >> 1) & (row >> 2) & (row >> 3) & (row >> 4) & (row >> 5)) != 0) {
            return true;
        }
    }

    for (int r = 0; r <= 13; ++r) {
        uint32_t v = bb.rows[r] & bb.rows[r + 1] & bb.rows[r + 2]
                   & bb.rows[r + 3] & bb.rows[r + 4] & bb.rows[r + 5];
        if (v != 0) return true;
    }

    for (int r = 0; r <= 13; ++r) {
        uint32_t d1 = bb.rows[r] & (bb.rows[r + 1] >> 1) & (bb.rows[r + 2] >> 2)
                    & (bb.rows[r + 3] >> 3) & (bb.rows[r + 4] >> 4) & (bb.rows[r + 5] >> 5);
        if (d1 != 0) return true;
    }

    for (int r = 0; r <= 13; ++r) {
        uint32_t d2 = bb.rows[r] & (bb.rows[r + 1] << 1) & (bb.rows[r + 2] << 2)
                    & (bb.rows[r + 3] << 3) & (bb.rows[r + 4] << 4) & (bb.rows[r + 5] << 5);
        if (d2 != 0) return true;
    }

    return false;
}

bool RuleChecker::isValidProMove(int moveCount, const Position& pos) {
   
    const int centerX = Board::WIDTH / 2;
    const int centerY = Board::HEIGHT / 2;

    if (moveCount == 1) {
        return pos.x == centerX && pos.y == centerY;
    }
    if (moveCount == 3) {
        bool insideCenter5x5 = (std::abs(pos.x - centerX) <= 2) && (std::abs(pos.y - centerY) <= 2);
        return !insideCenter5x5;
    }
    return true;
}

namespace {

    enum class Cell : char { EMPTY = '.', BLACK = 'X', WHITE = 'O', WALL = '#' };

    Cell getCell(const Board& board, int x, int y) {
        if (x < 0 || y < 0 || x >= Board::HEIGHT || y >= Board::WIDTH) {
            return Cell::WALL;
        }
        Position p{x, y};
        if (board.hasStone(p, Stone::BLACK)) return Cell::BLACK;
        if (board.hasStone(p, Stone::WHITE)) return Cell::WHITE;
        return Cell::EMPTY;
    }

    std::string extractLine(const Board& board, int cx, int cy, int dx, int dy, int radius) {
        std::string line;
        line.reserve(2 * radius + 1);
        for (int k = -radius; k <= radius; ++k) {
            line.push_back(static_cast<char>(getCell(board, cx + k * dx, cy + k * dy)));
        }
        return line;
    }


    bool hasFourThreat(const std::string& line) {
        for (size_t i = 0; i + 5 <= line.size(); ++i) {
            int blackCount = 0, emptyCount = 0;
            for (size_t j = i; j < i + 5; ++j) {
                if (line[j] == static_cast<char>(Cell::BLACK)) ++blackCount;
                else if (line[j] == static_cast<char>(Cell::EMPTY)) ++emptyCount;
            }
            if (blackCount == 4 && emptyCount == 1) return true;
        }
        return false;
    }


    bool hasLiveFour(const std::string& line) {
        const std::string pattern = ".XXXX.";
        return line.find(pattern) != std::string::npos;
    }


    bool isLiveThree(const std::string& line) {
        for (size_t i = 0; i < line.size(); ++i) {
            if (line[i] != static_cast<char>(Cell::EMPTY)) continue;
            std::string trial = line;
            trial[i] = static_cast<char>(Cell::BLACK);
            if (hasLiveFour(trial)) return true;
        }
        return false;
    }

} // namespace

bool RuleChecker::isRenjuFoul(const Board& board, const Position& pos) {
    Board tempBoard = board;
    tempBoard.placeStone(pos, Stone::BLACK);

    Bitboard blackBitboard = extractBitboard(tempBoard, Stone::BLACK);

    // Overline is always a foul.
    if (hasOverline(blackBitboard)) {
        return true;
    }

    // Completing an exact five always wins outright; never a foul.
    if (hasFiveInARow(blackBitboard)) {
        return false;
    }

    static const int dirs[4][2] = { {0, 1}, {1, 0}, {1, 1}, {1, -1} };
    const int radius = 6; 

    int fourCount = 0;
    int threeCount = 0;

    for (const auto& d : dirs) {
        std::string line = extractLine(tempBoard, pos.x, pos.y, d[0], d[1], radius);

        if (hasFourThreat(line)) {
            ++fourCount;
        }
        if (isLiveThree(line)) {
            ++threeCount;
        }
    }

    if (fourCount >= 2) {
        return true; // double-four
    }
    if (threeCount >= 2) {
        return true; // double-three
    }

    return false;
}



OpeningManager::OpeningManager(RuleVariant variant)
    : variant_(variant), swapPhase_(Swap2Phase::PLACE_INITIAL_3) {}

void OpeningManager::reset(RuleVariant newVariant) {
    variant_ = newVariant;
   
    swapPhase_ = (variant_ == RuleVariant::SWAP2 || variant_ == RuleVariant::SWAP)
                     ? Swap2Phase::PLACE_INITIAL_3
                     : Swap2Phase::FINISHED;
    openingStonesPlaced_ = 0;
}

void OpeningManager::notifyStonePlaced() {
    if (variant_ != RuleVariant::SWAP2 && variant_ != RuleVariant::SWAP) return;

    if (swapPhase_ == Swap2Phase::PLACE_INITIAL_3) {
        ++openingStonesPlaced_;
        if (openingStonesPlaced_ >= 3) {
            swapPhase_ = Swap2Phase::PLAYER2_CHOICE;
            openingStonesPlaced_ = 0;
        }
        return;
    }

    if (swapPhase_ == Swap2Phase::PLACE_EXTRA_2) {
        ++openingStonesPlaced_;
        if (openingStonesPlaced_ >= 2) {
            swapPhase_ = Swap2Phase::PLAYER1_CHOICE;
            openingStonesPlaced_ = 0;
        }
        return;
    }
}

bool OpeningManager::isOpeningFinished() const {
    if (variant_ != RuleVariant::SWAP2 && variant_ != RuleVariant::SWAP) return true;
    return swapPhase_ == Swap2Phase::FINISHED;
}

bool OpeningManager::processSwap2Choice(int choiceOption, Stone& currentTurn) {
    if (variant_ != RuleVariant::SWAP2 && variant_ != RuleVariant::SWAP) return false;

    if (swapPhase_ == Swap2Phase::PLAYER2_CHOICE) {
        if (choiceOption == 1 || choiceOption == 2) {
           
            swapPhase_ = Swap2Phase::FINISHED;
            currentTurn = Stone::WHITE;
            return true;
        }
        if (choiceOption == 3) {
            
            if (variant_ != RuleVariant::SWAP2) {
                return false;
            }
           
            swapPhase_ = Swap2Phase::PLACE_EXTRA_2;
            currentTurn = Stone::WHITE; 
            return true;
        }
        return false;
    }

    if (swapPhase_ == Swap2Phase::PLAYER1_CHOICE) {
       
        swapPhase_ = Swap2Phase::FINISHED;
        currentTurn = Stone::WHITE;
        return true;
    }

    return false;
}