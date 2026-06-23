// include/gomoku/Board.hpp
#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include "Position.hpp"
using namespace std;

class Board {
    public:
        static constexpr int WIDTH = 19;
        static constexpr int HEIGHT = 19;
        static constexpr int SIZE = 361;
        static constexpr int NUM_BITBOARDS = 6;  // 6 * 64 = 384 bits
        
        Board();
        ~Board();
        
        bool isEmpty(const Position& pos) const;
        bool hasBlue(const Position& pos) const;
        bool hasRed(const Position& pos) const;
        
        void placeBlue(const Position& pos);
        void placeRed(const Position& pos);
        void removeStone(const Position& pos);
        
        void reset();
        
        int countBlueStones() const;
        int countRedStones() const;
        
        vector<Position> getBlueStones() const;
        vector<Position> getRedStones() const;
        
        uint64_t getHash() const;
        
    private:
        int toIndex(const Position& pos) const {
            return pos.x * WIDTH + pos.y;
        }
        
        Position fromIndex(int index) const {
            return {index / WIDTH, index % WIDTH};
        }
        
        void getIndices(int pos, int& board_idx, int& bit_idx) const {
            board_idx = pos / 64;
            bit_idx = pos % 64; 
        }
        
        array<uint64_t, NUM_BITBOARDS> blue_;
        array<uint64_t, NUM_BITBOARDS> red_;
};