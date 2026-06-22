#include <gomoku/Board.hpp>

Board::Board() {
    reset();
}

bool Board::isInside(const Position& pos) const {
    return pos.x >= 0 && pos.x < WIDTH && pos.y >= 0 && pos.y < HEIGHT;
}

int Board::toIndex(const Position& pos) const {
    return pos.y * WIDTH + pos.x;
}

bool Board::isEmpty(const Position& pos) const {
    return isInside(pos) && cells_[toIndex(pos)] == Stone::EMPTY;
}

bool Board::placeStone(const Position& pos, Stone stone) {
    if (isInside(pos) && isEmpty(pos)) {
        cells_[toIndex(pos)] = stone;
        return true;
    }
    return false;
}

void Board::reset() {
    cells_.fill(Stone::EMPTY);
}

void Board::removeStone(const Position& pos) {
    if (isInside(pos)) {
        cells_[toIndex(pos)] = Stone::EMPTY;
    }
}

Stone Board::getStone(const Position& pos) const {
    if (isInside(pos)) {
        return cells_[toIndex(pos)];
    }
    return Stone::EMPTY;
}



Board::~Board() {}