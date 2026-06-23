#pragma once
#include <cstdint>
#include <vector>
#include "Position.hpp"
#include <bitset>

using namespace std;

class Board {
    public:
        static constexpr int WIDTH = 19;
        static constexpr int HEIGHT = 19;
        static constexpr int SIZE = 361;

        
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
        uint64_t getHash() const;
        
        vector<Position> getBlueStones() const;
        vector<Position> getRedStones() const;
        
        // uint64_t getBlue() const { return blue_; }
        // uint64_t getRed() const { return red_; }
        // void setBlue(uint64_t b) { blue_ = b; }
        // void setRed(uint64_t r) { red_ = r; }
        
    private:
        int toIndex(const Position& pos) const {
            return pos.x * WIDTH + pos.y;
        }
        
        Position fromIndex(int index) const {
            return {index / WIDTH, index % WIDTH};
        }
        
        bitset<SIZE> blue_ ;// Blue stones
        bitset<SIZE> red_ ;   // Red stones
};