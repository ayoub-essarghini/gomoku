#pragma once

#include <array>
#include <vector>
#include <cstdint>
#include "Position.hpp"
#include "Stone.hpp" 

using namespace std;

class Board {
    public:
        static constexpr int WIDTH = 19;
        static constexpr int HEIGHT = 19;
        static constexpr int SIZE = 361;
        static constexpr int NUM_BITBOARDS = 12;
        
        Board();
        ~Board();
        
        bool isEmpty(const Position& pos) const;
        bool hasStone(const Position& pos, Stone color) const;
        
        void placeStone(const Position& pos, Stone color);
        void removeStone(const Position& pos);
        
        void reset();
        
        int countStones(Stone color) const;
        vector<Position> getStones(Stone color) const;
        
    private:
        bool isValid(const Position& pos) const {
            return pos.x >= 0 && pos.x < HEIGHT && pos.y >= 0 && pos.y < WIDTH;
        }

        int toIndex(const Position& pos) const {
            return pos.x * WIDTH + pos.y;
        }
        
        Position fromIndex(int index) const {
            return {index / WIDTH, index % WIDTH};
        }
        
        Stone getState(int pos) const {
            int bit_pair = pos * 2;
            int board_idx = bit_pair / 64;
            int bit_idx = bit_pair % 64;
            
            uint64_t val = (board_[board_idx] >> bit_idx) & 3ULL;
            return static_cast<Stone>(val);
        }
        
        void setState(int pos, Stone state) {
            int bit_pair = pos * 2;
            int board_idx = bit_pair / 64;
            int bit_idx = bit_pair % 64;
            
            uint64_t mask = 3ULL << bit_idx;
            board_[board_idx] &= ~mask;
            
            uint64_t state_val = static_cast<uint64_t>(state);
            board_[board_idx] |= (state_val << bit_idx);
        }
        
        array<uint64_t, NUM_BITBOARDS> board_{};
};