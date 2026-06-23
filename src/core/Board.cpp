#include <gomoku/Board.hpp>

Board::Board() {
    reset();
}

Board::~Board() {}

bool Board::isEmpty(const Position& pos) const {
    if (!isValid(pos)) return false;
    return getState(toIndex(pos)) == Stone::EMPTY;
}

bool Board::hasStone(const Position& pos, Stone color) const {
    if (!isValid(pos)) return false;
    return getState(toIndex(pos)) == color;
}

void Board::placeStone(const Position& pos, Stone color) {
    if (!isValid(pos)) return;
    setState(toIndex(pos), color);
}

void Board::removeStone(const Position& pos) {
    if (!isValid(pos)) return;
    setState(toIndex(pos), Stone::EMPTY);
}

void Board::reset() {
    board_.fill(0); 
}

int Board::countStones(Stone color) const {
    int count = 0;
    for (int i = 0; i < SIZE; i++) {
        if (getState(i) == color) count++;
    }
    return count;
}

vector<Position> Board::getStones(Stone color) const {
    vector<Position> stones;
    stones.reserve(SIZE / 2); 
    for (int i = 0; i < SIZE; i++) {
        if (getState(i) == color) {
            stones.push_back(fromIndex(i));
        }
    }
    return stones;
}