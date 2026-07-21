#pragma once

#include <gomoku/ui/UIBoard.hpp>

class Game {
public:
    Game() = default;
    ~Game() = default;

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void start();

private:
    UIBoard uiBoard_;
};