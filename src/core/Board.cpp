#include <gomoku/Board.hpp>

Board::Board() {
    reset();
}

Board::~Board() {}

bool Board::isEmpty(const Position& pos) const {
    if (pos.x < 0 || pos.x >= HEIGHT || pos.y < 0 || pos.y >= WIDTH)
        return false;
    
    int idx = toIndex(pos);
    int board_idx, bit_idx;
    getIndices(idx, board_idx, bit_idx);
    
    uint64_t blue_bit = (blue_[board_idx] >> bit_idx) & 1;
    uint64_t red_bit = (red_[board_idx] >> bit_idx) & 1;
    
    return !blue_bit && !red_bit;
}

bool Board::hasBlue(const Position& pos) const {
    if (pos.x < 0 || pos.x >= HEIGHT || pos.y < 0 || pos.y >= WIDTH)
        return false;
    
    int idx = toIndex(pos);
    int board_idx, bit_idx;
    getIndices(idx, board_idx, bit_idx);
    
    return (blue_[board_idx] >> bit_idx) & 1;
}

bool Board::hasRed(const Position& pos) const {
    if (pos.x < 0 || pos.x >= HEIGHT || pos.y < 0 || pos.y >= WIDTH)
        return false;
    
    int idx = toIndex(pos);
    int board_idx, bit_idx;
    getIndices(idx, board_idx, bit_idx);
    
    return (red_[board_idx] >> bit_idx) & 1;
}

void Board::placeBlue(const Position& pos) {
    if (pos.x < 0 || pos.x >= HEIGHT || pos.y < 0 || pos.y >= WIDTH)
        return;
    
    int idx = toIndex(pos);
    int board_idx, bit_idx;
    getIndices(idx, board_idx, bit_idx);
    
    blue_[board_idx] |= (1ULL << bit_idx);
}

void Board::placeRed(const Position& pos) {
    if (pos.x < 0 || pos.x >= HEIGHT || pos.y < 0 || pos.y >= WIDTH)
        return;
    
    int idx = toIndex(pos);
    int board_idx, bit_idx;
    getIndices(idx, board_idx, bit_idx);
    
    red_[board_idx] |= (1ULL << bit_idx);
}

void Board::removeStone(const Position& pos) {
    if (pos.x < 0 || pos.x >= HEIGHT || pos.y < 0 || pos.y >= WIDTH)
        return;
    
    int idx = toIndex(pos);
    int board_idx, bit_idx;
    getIndices(idx, board_idx, bit_idx);
    
    uint64_t mask = 1ULL << bit_idx;
    blue_[board_idx] &= ~mask;
    red_[board_idx] &= ~mask;
}

void Board::reset() {
    for (int i = 0; i < NUM_BITBOARDS; i++) {
        blue_[i] = 0;
        red_[i] = 0;
    }
}

int Board::countBlueStones() const {
    int count = 0;
    for (int i = 0; i < NUM_BITBOARDS; i++) {
        count += __builtin_popcountll(blue_[i]);
    }
    return count;
}

int Board::countRedStones() const {
    int count = 0;
    for (int i = 0; i < NUM_BITBOARDS; i++) {
        count += __builtin_popcountll(red_[i]);
    }
    return count;
}

vector<Position> Board::getBlueStones() const {
    vector<Position> stones;
    
    for (int board_idx = 0; board_idx < NUM_BITBOARDS; board_idx++) {
        uint64_t bits = blue_[board_idx];
        
        while (bits) {
            int bit_idx = __builtin_ctzll(bits);  
            int idx = board_idx * 64 + bit_idx;
            
            if (idx < SIZE)
                stones.push_back(fromIndex(idx));
            
            bits &= bits - 1;  
        }
    }
    
    return stones;
}

vector<Position> Board::getRedStones() const {
    vector<Position> stones;
    
    for (int board_idx = 0; board_idx < NUM_BITBOARDS; board_idx++) {
        uint64_t bits = red_[board_idx];
        
        while (bits) {
            int bit_idx = __builtin_ctzll(bits);  
            int idx = board_idx * 64 + bit_idx;
            
            if (idx < SIZE)
                stones.push_back(fromIndex(idx));
            
            bits &= bits - 1;  
        }
    }
    
    return stones;
}

uint64_t Board::getHash() const {
    uint64_t hash = 0;
    for (int i = 0; i < NUM_BITBOARDS; i++) {
        hash ^= blue_[i] ^ red_[i]; 
    }
    return hash;
}