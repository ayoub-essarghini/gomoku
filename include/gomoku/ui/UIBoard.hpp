#pragma once

#include <SFML/Graphics.hpp>
#include <gomoku/Board.hpp>
#include <gomoku/Rules.hpp>
#include <vector>
#include <string>
#include <iostream>

struct Particle {
    sf::Vector2f position;
    sf::Vector2f velocity;
    float radius;
    sf::Color color;
};

enum class GameMode {
    HumanVsHuman,
    HumanVsAI
};

enum class Screen {
    ModeSelect,
    Game
};

class UIBoard {
public:
    UIBoard();
    ~UIBoard();

    void init();
    void resetGameState();
    std::string getWinnerName() const;

private:
    sf::RenderWindow window_;
    sf::Font font_;
    sf::Vector2i mousePosition_;
    bool mousePressed_{false};

    Screen screen_{Screen::ModeSelect};
    GameMode mode_{GameMode::HumanVsHuman};

    Board board_;
    Stone currentTurn_{Stone::BLACK};
    bool gameOver_{false};

    RuleVariant ruleVariant_{RuleVariant::STANDARD};
    OpeningManager openingManager_{RuleVariant::STANDARD};
    std::string statusMessage_{"Game in progress"};
    std::string winnerName_{""};

    int blackCaptures_{0};
    int whiteCaptures_{0};
    float lastAiTimeMs_{0.f};

   
    std::vector<Particle> particles_;

    bool handleClick(sf::Vector2i mousePos);
    bool handleModeSelectClick(sf::Vector2i mousePos);
    bool handlePanelClick(sf::Vector2i mousePos);

    void drawBoard();
    void drawLines();
    void drawStone(const Position &pos, Stone color);
    void drawPanel();
    void drawText(const std::string &text, float x, float y, unsigned int size);
    void drawModeSelect();

    void initParticles();
    void updateParticles();
    void drawParticles();

    bool isInsideRect(sf::Vector2i mousePos, const sf::FloatRect &rect) const;
    sf::Color getButtonColor(const sf::FloatRect &rect) const;
};