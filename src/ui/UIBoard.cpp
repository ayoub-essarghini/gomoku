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
    init();
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
    window_.setFramerateLimit(60);

    while (window_.isOpen())
    {
        while (const auto event = window_.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window_.close();
            }
            if (const auto *mouse = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouse->button == sf::Mouse::Button::Left)
                {
                    auto pos = sf::Mouse::getPosition(window_);
                    handleClick(pos);
                }
            }
        }

        window_.clear();
        drawBoard();
        drawPanel();

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

void UIBoard::drawText(const std::string& text, float x, float y, unsigned int size)
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

UIBoard::~UIBoard()
{
    window_.close();
}

