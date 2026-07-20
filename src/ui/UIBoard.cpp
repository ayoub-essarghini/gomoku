#include <gomoku/ui/UIBoard.hpp>

UIBoard::UIBoard()
{
    board_ = Board();
    init();
}

void UIBoard::init()
{
    window_.create(sf::VideoMode({800u, 800u}), "Gomoku");
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
        
        window_.display();
    }
}

bool UIBoard::handleClick(sf::Vector2i mousePos)
{
    constexpr float size = 800.f;
    constexpr float step = size / 18.f;
    constexpr int maxIndex = 18;

    int col = static_cast<int>(std::round(mousePos.x / step));
    int row = static_cast<int>(std::round(mousePos.y / step));

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
    sf::RectangleShape boardBg(sf::Vector2f(800.f, 800.f));

    boardBg.setFillColor(sf::Color(222, 184, 135));
    window_.draw(boardBg);
    drawLines();
    for (const auto& pos : board_.getStones(Stone::BLACK))
        drawStone(pos, Stone::BLACK);

    for (const auto& pos : board_.getStones(Stone::WHITE))
        drawStone(pos, Stone::WHITE);
}

void UIBoard::drawLines()
{
    constexpr float size = 800.f;
    constexpr float step = size / 18.f;

    for (int i = 0; i < 19; ++i)
    {
        float x = i * step;
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(x, 0.f), sf::Color::Black),
            sf::Vertex(sf::Vector2f(x, size), sf::Color::Black)};
        window_.draw(line, 2, sf::PrimitiveType::Lines);
    }

    for (int i = 0; i < 19; ++i)
    {
        float y = i * step;
        sf::Vertex line[] = {
            sf::Vertex(sf::Vector2f(0.f, y), sf::Color::Black),
            sf::Vertex(sf::Vector2f(size, y), sf::Color::Black)};
        window_.draw(line, 2, sf::PrimitiveType::Lines);
    }
}

void UIBoard::drawStone(const Position &pos, Stone color)
{
    constexpr float size = 800.f;
    constexpr float step = size / 18.f;
    constexpr float radius = step * 0.4f;

    sf::CircleShape stone(radius);
    stone.setPosition(sf::Vector2f(pos.y * step - radius, pos.x * step - radius));
    stone.setFillColor(color == Stone::BLACK ? sf::Color::Black : sf::Color::White);
    stone.setOutlineThickness(1.f);
    stone.setOutlineColor(sf::Color::Black);

    window_.draw(stone);
}

UIBoard::~UIBoard()
{
    window_.close();
}