#pragma once
#include <array>
#include "Stone.hpp"
#include "Position.hpp"
using namespace std;


class Board {
    public:
        static constexpr int WIDTH = 19;
        static constexpr int HEIGHT = 19;
        static constexpr int SIZE = WIDTH * HEIGHT;
        Board();
        ~Board();
        bool isInside(const Position& pos) const;
        bool isEmpty(const Position& pos) const;
        Stone getStone(const Position& pos) const;
        bool placeStone(const Position& pos, Stone stone);
        void removeStone(const Position& pos);
        void reset();

    private:
        int toIndex(const Position& pos) const;
        array<Stone,SIZE> cells_;

};