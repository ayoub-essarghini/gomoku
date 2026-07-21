#include <gomoku/ui/UIBoard.hpp>

namespace
{
    constexpr float BOARD_OFFSET_X = 40.f;
    constexpr float BOARD_OFFSET_Y = 40.f;
    constexpr float CELL_SIZE = 40.f;
    constexpr float BOARD_PIXEL_SIZE = CELL_SIZE * 18.f;
    constexpr float STONE_RADIUS = CELL_SIZE * 0.42f;
}

UIBoard::UIBoard()
{
    board_ = Board();
   
}

void UIBoard::resetGameState()
{
    board_.reset();
    currentTurn_ = Stone::BLACK;
    mousePressed_ = false;
    gameOver_ = false;
    blackCaptures_ = 0;
    whiteCaptures_ = 0;
    lastAiTimeMs_ = 0.f;
}

std::string UIBoard::getWinnerName() const
{
    return currentTurn_ == Stone::BLACK ? "White" : "Black";
}

void UIBoard::init()
{
    window_.create(sf::VideoMode({1100u, 860u}),
                   "Gomoku",
                   sf::Style::Titlebar | sf::Style::Close

    );
    if (!font_.openFromFile("src/assets/font/retro_gaming.ttf"))
    {
        std::cerr << "Failed to load font\n";
    }
    initParticles();
    window_.setFramerateLimit(60);

    while (window_.isOpen())
    {
        while (const auto event = window_.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window_.close();
                return ;
            }
            if (const auto *mouse = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouse->button == sf::Mouse::Button::Left)
                {
                    mousePressed_ = true;

                    auto pos = sf::Mouse::getPosition(window_);
                    if (screen_ == Screen::ModeSelect)
                        handleModeSelectClick(pos);
                    else
                        handleClick(pos);
                }
            }
            if (const auto *mouse = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (mouse->button == sf::Mouse::Button::Left)
                {
                    mousePressed_ = false;
                }
            }
        }
        mousePosition_ = sf::Mouse::getPosition(window_);
        window_.clear();
        if (screen_ == Screen::ModeSelect)
        {
            drawModeSelect();
        }
        else
        {
            drawBoard();
            drawPanel();
        }

        window_.display();
    }
}

bool UIBoard::handleClick(sf::Vector2i mousePos)
{
    if (gameOver_)
        return false;
    constexpr int maxIndex = 18;

    int col = static_cast<int>(std::round((mousePos.x - BOARD_OFFSET_X) / CELL_SIZE));
    int row = static_cast<int>(std::round((mousePos.y - BOARD_OFFSET_Y) / CELL_SIZE));

    if (col < 0 || col > maxIndex || row < 0 || row > maxIndex)
        return false;

    Position boardPos{row, col};

    if (!board_.placeStone(boardPos, currentTurn_))
        return false;

    currentTurn_ = currentTurn_ == Stone::BLACK ? Stone::WHITE : Stone::BLACK;
    return true;
}

void UIBoard::drawBoard()
{
    sf::RectangleShape boardBg(sf::Vector2f(BOARD_PIXEL_SIZE, BOARD_PIXEL_SIZE));
    boardBg.setPosition({BOARD_OFFSET_X, BOARD_OFFSET_Y});
    boardBg.setFillColor(sf::Color(222, 184, 135));
    window_.draw(boardBg);
    drawLines();
    for (const auto &pos : board_.getStones(Stone::BLACK))
        drawStone(pos, Stone::BLACK);

    for (const auto &pos : board_.getStones(Stone::WHITE))
        drawStone(pos, Stone::WHITE);
}

void UIBoard::drawLines()
{
    for (int i = 0; i < 19; ++i)
    {
        float x = BOARD_OFFSET_X + i * CELL_SIZE;

        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x, BOARD_OFFSET_Y), sf::Color::Black),
            sf::Vertex(sf::Vector2f(x, BOARD_OFFSET_Y + BOARD_PIXEL_SIZE), sf::Color::Black)};

        window_.draw(line, 2, sf::PrimitiveType::Lines);
    }

    for (int i = 0; i < 19; ++i)
    {
        float y = BOARD_OFFSET_Y + i * CELL_SIZE;

        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(BOARD_OFFSET_X, y), sf::Color::Black),
            sf::Vertex(sf::Vector2f(BOARD_OFFSET_X + BOARD_PIXEL_SIZE, y), sf::Color::Black)};

        window_.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void UIBoard::drawStone(const Position &pos, Stone color)
{

    sf::CircleShape stone(STONE_RADIUS);

    float px = BOARD_OFFSET_X + pos.y * CELL_SIZE;
    float py = BOARD_OFFSET_Y + pos.x * CELL_SIZE;

    stone.setPosition(sf::Vector2f(px - STONE_RADIUS, py - STONE_RADIUS));
    stone.setFillColor(color == Stone::BLACK ? sf::Color::Black : sf::Color::White);
    stone.setOutlineThickness(1.f);
    stone.setOutlineColor(sf::Color::Black);

    window_.draw(stone);
}

void UIBoard::drawText(const std::string &text, float x, float y, unsigned int size)
{
    sf::Text label(font_, text, size);
    label.setFillColor(sf::Color(245, 245, 245));
    label.setPosition({x, y});
    window_.draw(label);
}

void UIBoard::drawPanel()
{
    sf::RectangleShape panel(sf::Vector2f(300.f, 860.f));
    panel.setPosition({800.f, 0.f});
    panel.setFillColor(sf::Color(38, 43, 48));
    window_.draw(panel);

    std::string turn = currentTurn_ == Stone::BLACK ? "Black" : "White";
    std::string mode = mode_ == GameMode::HumanVsHuman ? "Human vs Human" : "Human vs AI";

    drawText("Gomoku", 830.f, 40.f, 32);
    drawText("Turn: " + turn, 830.f, 100.f, 22);
    drawText("Mode: " + mode, 830.f, 140.f, 18);

    drawText("Captures", 830.f, 200.f, 22);
    drawText("Black: " + std::to_string(blackCaptures_), 830.f, 235.f, 18);
    drawText("White: " + std::to_string(whiteCaptures_), 830.f, 265.f, 18);

    drawText("AI time: " + std::to_string(static_cast<int>(lastAiTimeMs_)) + " ms", 830.f, 330.f, 18);
    drawText("Suggestion: --", 830.f, 370.f, 18);

    drawText("[ New Game ]", 830.f, 460.f, 20);
    drawText("[ Suggest Move ]", 830.f, 500.f, 20);
    drawText("[ Mode ]", 830.f, 540.f, 20);
}

bool UIBoard::isInsideRect(sf::Vector2i mousePos, const sf::FloatRect &rect) const
{
    return rect.contains(sf::Vector2f(
        static_cast<float>(mousePos.x),
        static_cast<float>(mousePos.y)));
}

sf::Color UIBoard::getButtonColor(const sf::FloatRect &rect) const
{
    bool hovered = isInsideRect(mousePosition_, rect);

    if (hovered && mousePressed_)
        return sf::Color(170, 130, 85);

    if (hovered)
        return sf::Color(238, 200, 145);

    return sf::Color(222, 184, 135);
}

bool UIBoard::handleModeSelectClick(sf::Vector2i mousePos)
{
    sf::FloatRect humanVsHumanButton({350.f, 320.f}, {400.f, 70.f});
    sf::FloatRect humanVsAiButton({350.f, 420.f}, {400.f, 70.f});

    if (isInsideRect(mousePos, humanVsHumanButton))
    {
        mode_ = GameMode::HumanVsHuman;
        screen_ = Screen::Game;
        return true;
    }

    if (isInsideRect(mousePos, humanVsAiButton))
    {
        mode_ = GameMode::HumanVsAI;
        screen_ = Screen::Game;
        return true;
    }

    return false;
}

void UIBoard::drawModeSelect()
{
    window_.clear(sf::Color(28, 32, 36));

    updateParticles();
    drawParticles();

    sf::RectangleShape titleBar(sf::Vector2f(1100.f, 140.f));
    titleBar.setPosition({0.f, 0.f});
    titleBar.setFillColor(sf::Color(38, 43, 48));
    window_.draw(titleBar);

    drawText("Gomoku", 420.f, 45.f, 48);
    drawText("Choose Mode", 430.f, 190.f, 28);

    sf::FloatRect humanVsHumanRect({350.f, 320.f}, {400.f, 70.f});

    sf::RectangleShape humanVsHumanButton(sf::Vector2f(400.f, 70.f));
    humanVsHumanButton.setPosition({350.f, 320.f});
    humanVsHumanButton.setFillColor(getButtonColor(humanVsHumanRect));
    humanVsHumanButton.setOutlineThickness(2.f);
    humanVsHumanButton.setOutlineColor(sf::Color(245, 245, 245));
    window_.draw(humanVsHumanButton);

    sf::FloatRect humanVsAiRect({350.f, 420.f}, {400.f, 70.f});

    sf::RectangleShape humanVsAiButton(sf::Vector2f(400.f, 70.f));
    humanVsAiButton.setPosition({350.f, 420.f});
    humanVsAiButton.setFillColor(getButtonColor(humanVsAiRect));
    humanVsAiButton.setOutlineThickness(2.f);
    humanVsAiButton.setOutlineColor(sf::Color(245, 245, 245));
    window_.draw(humanVsAiButton);

    sf::Text humanVsHumanText(font_, "Human vs Human", 24);
    humanVsHumanText.setFillColor(sf::Color(28, 32, 36));
    humanVsHumanText.setPosition({440.f, 342.f});
    window_.draw(humanVsHumanText);

    sf::Text humanVsAiText(font_, "Human vs AI", 24);
    humanVsAiText.setFillColor(sf::Color(28, 32, 36));
    humanVsAiText.setPosition({475.f, 442.f});
    window_.draw(humanVsAiText);
}

// Particles effects

void UIBoard::initParticles()
{
    particles_.clear();

    for (int i = 0; i < 60; ++i)
    {
        Particle p;

        p.position = sf::Vector2f(
            static_cast<float>(std::rand() % 1100),
            static_cast<float>(std::rand() % 860));

        p.velocity = sf::Vector2f(
            -0.2f + static_cast<float>(std::rand() % 40) / 100.f,
            0.1f + static_cast<float>(std::rand() % 30) / 100.f);

        p.radius = 1.f + static_cast<float>(std::rand() % 3);

        p.color = sf::Color(222, 184, 135, 120);

        particles_.push_back(p);
    }
}

void UIBoard::updateParticles()
{
    for (auto &p : particles_)
    {
        p.position += p.velocity;

        if (p.position.x < 0.f)
            p.position.x = 1100.f;
        if (p.position.x > 1100.f)
            p.position.x = 0.f;
        if (p.position.y < 0.f)
            p.position.y = 860.f;
        if (p.position.y > 860.f)
            p.position.y = 0.f;
    }
}

void UIBoard::drawParticles()
{
    for (const auto &p : particles_)
    {
        sf::CircleShape dot(p.radius);
        dot.setPosition(p.position);
        dot.setFillColor(p.color);
        window_.draw(dot);
    }
}

UIBoard::~UIBoard()
{
    if (window_.isOpen())
        window_.close();
}
