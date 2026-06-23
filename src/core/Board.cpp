#include <gomoku/Board.hpp>

Board::Board() : blue_(0), red_(0) {}
Board::~Board() {}

bool Board::isEmpty(const Position& pos) const {
    int index = toIndex(pos);
    uint64_t mask = 1ULL << index;
    return !((blue_ & mask) | (red_ & mask));
}

bool Board::hasBlue(const Position& pos) const {
    int index = toIndex(pos);
    return (blue_ >> index) & 1;
}

bool Board::hasRed(const Position& pos) const {
    int index = toIndex(pos);
    return (red_ >> index) & 1;
}

void Board::placeBlue(const Position& pos) {
    int index = toIndex(pos);
    blue_ |= (1ULL << index);
}

void Board::placeRed(const Position& pos) {
    int index = toIndex(pos);
    red_ |= (1ULL << index);
}

void Board::removeStone(const Position& pos) {
    int index = toIndex(pos);
    uint64_t mask = 1ULL << index;
    blue_ &= ~mask;
    red_ &= ~mask;
}

void Board::reset() {
    blue_ = 0;
    red_ = 0;
}

int Board::countBlueStones() const {
    return __builtin_popcountll(blue_);
}

int Board::countRedStones() const {
    return __builtin_popcountll(red_);
}

uint64_t Board::getHash() const {
    return blue_ ^ red_;
}

vector<Position> Board::getBlueStones() const {
    vector<Position> stones;
    uint64_t board = blue_;
    while (board) {
        int index = __builtin_ctzll(board);
        stones.push_back(fromIndex(index));
        board &= board - 1;
    }
    return stones;
}

vector<Position> Board::getRedStones() const {
    vector<Position> stones;
    uint64_t board = red_;
    while (board) {
        int index = __builtin_ctzll(board);
        stones.push_back(fromIndex(index));
        board &= board - 1;
    }
    return stones;
}