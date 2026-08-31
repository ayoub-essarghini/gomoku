#include <gomoku/ui/UIBoard.hpp>
#include <gomoku/Rules.hpp>
#include <chrono>
#include <iostream>
#include <cmath>
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
    resetGameState();
}

std::optional<Suggestion> UIBoard::requestSuggestion()
{
    if (gameOver_)
        return std::nullopt;

    auto start = std::chrono::steady_clock::now();

    Position best = ai_.findBestMove(
        board_,
        currentTurn_,
        ruleVariant_,
        blackCaptures_,
        whiteCaptures_);

    auto end = std::chrono::steady_clock::now();
    int elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();

    if (best.x < 0 || best.y < 0)
        return std::nullopt;

    return Suggestion{ best, elapsed };
}



void UIBoard::resetGameState()
{
    board_.reset();
    openingManager_.reset(ruleVariant_);
    currentTurn_ = Stone::BLACK;
    mousePressed_ = false;
    gameOver_ = false;
    blackCaptures_ = 0;
    whiteCaptures_ = 0;
    lastAiTimeMs_ = 0.f;
    hasForbiddenMarker_ = false;
    forbiddenPos_ = Position{-1, -1};
    statusMessage_ = "Game in progress";
    winnerName_ = "";
    winningLine_ = WinLine{};
}

std::string UIBoard::getWinnerName() const
{
    if (!winnerName_.empty())
        return winnerName_;
    return currentTurn_ == Stone::BLACK ? "White" : "Black";
}

void UIBoard::init()
{
    window_.create(sf::VideoMode({1100u, 860u}),
                   "Gomoku Engine",
                   sf::Style::Titlebar | sf::Style::Close);

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
                return;
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
        // Render the frame (with any stone just placed by the human) before
        // performAiTurn() blocks the loop for up to the AI's time budget —
        // otherwise the human's move never appears on screen until the AI
        // has already finished thinking.
        if (isAiTurnPending())
            statusMessage_ = "AI is thinking";

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

            if (gameOver_)
                drawGameOverOverlay();
        }

        window_.display();

        performAiTurn();
    }
}


bool UIBoard::handlePanelClick(sf::Vector2i mousePos)
{
    // [ New Game ] Button Area
    if (mousePos.x >= 830 && mousePos.x <= 1030 && mousePos.y >= 460 && mousePos.y <= 490)
    {
        resetGameState();
        return true;
    }

    // [ Mode ] Button Area
    if (mousePos.x >= 830 && mousePos.x <= 1030 && mousePos.y >= 540 && mousePos.y <= 570)
    {
        screen_ = Screen::ModeSelect;
        // resetGameState();
        return true;
    }

    if (mousePos.x >= 830 && mousePos.x <= 1030 && mousePos.y >= 180 && mousePos.y <= 205)
    {
        uint8_t nextRule = (static_cast<uint8_t>(ruleVariant_) + 1) % 5;

        while (mode_ == GameMode::HumanVsAI &&
               (nextRule == static_cast<uint8_t>(RuleVariant::SWAP) ||
                nextRule == static_cast<uint8_t>(RuleVariant::SWAP2)))
        {
            nextRule = (nextRule + 1) % 5;
        }

        ruleVariant_ = static_cast<RuleVariant>(nextRule);
        resetGameState();
        return true;
    }

    // Swap2 Decision Phase Button Interactions
    if (ruleVariant_ == RuleVariant::SWAP2 && !openingManager_.isOpeningFinished())
    {
        Swap2Phase phase = openingManager_.getSwap2Phase();

        if (phase == Swap2Phase::PLAYER2_CHOICE)
        {
            if (mousePos.y >= 600 && mousePos.y <= 625)
            {
                openingManager_.processSwap2Choice(1, currentTurn_);
                statusMessage_ = "P2: Stay White";
            }
            else if (mousePos.y >= 630 && mousePos.y <= 655)
            {
                openingManager_.processSwap2Choice(2, currentTurn_);
                statusMessage_ = "P2: Swapped to Black";
            }
            else if (mousePos.y >= 660 && mousePos.y <= 685)
            {
                openingManager_.processSwap2Choice(3, currentTurn_);
                statusMessage_ = "P2: Placing 2 more stones";
            }
            return true;
        }
        if (phase == Swap2Phase::PLAYER1_CHOICE)
        {
            if (mousePos.y >= 600 && mousePos.y <= 625)
            {
                openingManager_.processSwap2Choice(1, currentTurn_);
                statusMessage_ = "P1: Playing Black";
            }
            else if (mousePos.y >= 630 && mousePos.y <= 655)
            {
                openingManager_.processSwap2Choice(2, currentTurn_);
                statusMessage_ = "P1: Playing White";
            }
            if (openingManager_.isOpeningFinished())
            {
                statusMessage_ = "Game in progress";
            }
            return true;
        }
    }

    if (ruleVariant_ == RuleVariant::SWAP && !openingManager_.isOpeningFinished())
    {
        if (openingManager_.getSwap2Phase() == Swap2Phase::PLAYER2_CHOICE)
        {
            if (mousePos.y >= 600 && mousePos.y <= 625)
            {
                openingManager_.processSwap2Choice(1, currentTurn_);
                statusMessage_ = "Game in progress";
            }
            else if (mousePos.y >= 630 && mousePos.y <= 655)
            {
                openingManager_.processSwap2Choice(2, currentTurn_);
                statusMessage_ = "Game in progress";
            }
            return true;
        }
    }

    if (mousePos.x >= 830 && mousePos.x <= 1030 && mousePos.y >= 610 && mousePos.y <= 635)
    {
        uint8_t next = (static_cast<uint8_t>(difficulty_) + 1) % 3;
        difficulty_ = static_cast<Difficulty>(next);
        ai_.setDifficulty(difficulty_);
        return true;
    }

    return false;
}

bool UIBoard::handleClick(sf::Vector2i mousePos)
{
    // Route panel interaction
    if (mousePos.x >= 800)
    {
        return handlePanelClick(mousePos);
    }

    if (gameOver_)
        return false;

    if (mode_ == GameMode::HumanVsAI && currentTurn_ == aiColor_)
    {
        statusMessage_ = "AI is thinking";
        return false;
    }

    const bool inOpening = (ruleVariant_ == RuleVariant::SWAP2 || ruleVariant_ == RuleVariant::SWAP)
                            && !openingManager_.isOpeningFinished();

   
    if (inOpening)
    {
        Swap2Phase phase = openingManager_.getSwap2Phase();
        if (phase == Swap2Phase::PLAYER2_CHOICE || phase == Swap2Phase::PLAYER1_CHOICE)
        {
            statusMessage_ = "Waiting for opening decision (see panel)";
            return false;
        }
    }

    constexpr int maxIndex = 18;
    int col = static_cast<int>(std::round((mousePos.x - BOARD_OFFSET_X) / CELL_SIZE));
    int row = static_cast<int>(std::round((mousePos.y - BOARD_OFFSET_Y) / CELL_SIZE));

    if (col < 0 || col > maxIndex || row < 0 || row > maxIndex)
        return false;

    return playMove(Position{row, col});
}

bool UIBoard::playMove(const Position &boardPos)
{
    if (gameOver_)
        return false;

    const bool inOpening = (ruleVariant_ == RuleVariant::SWAP2 || ruleVariant_ == RuleVariant::SWAP)
                            && !openingManager_.isOpeningFinished();

    int currentMoveCount = board_.countStones(Stone::BLACK) + board_.countStones(Stone::WHITE) + 1;

    // 1. Validate Pro Rule Conditions
    if (ruleVariant_ == RuleVariant::PRO && !RuleChecker::isValidProMove(currentMoveCount, boardPos))
    {
        statusMessage_ = (currentMoveCount == 1) ? "Pro: Move 1 must be center!"
                                                 : "Pro: Move 3 outside 5x5 center!";
        return false;
    }

    if (ruleVariant_ == RuleVariant::RENJU && currentTurn_ == Stone::BLACK)
    {
        if (RuleChecker::isRenjuFoul(board_, boardPos))
        {
            hasForbiddenMarker_ = true;
            forbiddenPos_ = boardPos;
            statusMessage_ = "Forbidden Move";
            return false;
        }
    }

    if (!board_.placeStone(boardPos, currentTurn_))
        return false;

   
    int captured = RuleChecker::applyCaptures(board_, boardPos, currentTurn_);
    if (captured > 0)
    {
        if (currentTurn_ == Stone::BLACK)
            blackCaptures_ += captured;
        else
            whiteCaptures_ += captured;

        if ((currentTurn_ == Stone::BLACK && blackCaptures_ >= 5) ||
            (currentTurn_ == Stone::WHITE && whiteCaptures_ >= 5))
        {
            gameOver_ = true;
            winnerName_ = (currentTurn_ == Stone::BLACK) ? "Black" : "White";
            statusMessage_ = winnerName_ + " Wins by Captures!";
            return true;
        }
    }

    if (inOpening)
    {
        
        openingManager_.notifyStonePlaced();

        Swap2Phase newPhase = openingManager_.getSwap2Phase();
        if (newPhase == Swap2Phase::PLAYER2_CHOICE)
        {
            statusMessage_ = "Player 2: choose an option";
            return true;
        }
        if (newPhase == Swap2Phase::PLAYER1_CHOICE)
        {
            statusMessage_ = "Player 1: choose a color";
            return true;
        }
    }

    Bitboard currentBB = RuleChecker::extractBitboard(board_, currentTurn_);

    if (RuleChecker::hasFiveInARow(currentBB))
    {
        if (ruleVariant_ == RuleVariant::RENJU && currentTurn_ == Stone::BLACK && RuleChecker::hasOverline(currentBB))
        {
            gameOver_ = true;
            winnerName_ = "White";
            statusMessage_ = "Foul: Black created Overline (6+)";
        }
        else
        {
            gameOver_ = true;
            winnerName_ = (currentTurn_ == Stone::BLACK) ? "Black" : "White";
            statusMessage_ = winnerName_ + " Wins!";
            winningLine_ = RuleChecker::findWinningLine(currentBB);
        }
        return true;
    }

    // Check Board Full Draw Condition
    if (currentMoveCount == 361)
    {
        gameOver_ = true;
        winnerName_ = "None";
        statusMessage_ = "Draw Game!";
        return true;
    }

    // Turn transition
    currentTurn_ = (currentTurn_ == Stone::BLACK) ? Stone::WHITE : Stone::BLACK;
    statusMessage_ = "Game in progress";
    return true;
}

bool UIBoard::isAiTurnPending() const
{
    return screen_ == Screen::Game &&
           mode_ == GameMode::HumanVsAI &&
           !gameOver_ &&
           currentTurn_ == aiColor_ &&
           ruleVariant_ != RuleVariant::SWAP &&
           ruleVariant_ != RuleVariant::SWAP2 &&
           openingManager_.isOpeningFinished();
}

void UIBoard::performAiTurn()
{
    if (screen_ != Screen::Game ||
        mode_ != GameMode::HumanVsAI ||
        gameOver_ ||
        currentTurn_ != aiColor_)
    {
        return;
    }

    if (ruleVariant_ == RuleVariant::SWAP || ruleVariant_ == RuleVariant::SWAP2)
    {
        statusMessage_ = "AI unavailable for Swap openings";
        return;
    }

    if (!openingManager_.isOpeningFinished())
        return;

    statusMessage_ = "AI is thinking";
    const auto startTime = std::chrono::steady_clock::now();

    const Position selectedMove = ai_.findBestMove(
        board_,
        aiColor_,
        ruleVariant_,
        blackCaptures_,
        whiteCaptures_);

    const auto endTime = std::chrono::steady_clock::now();
    lastAiTimeMs_ = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (selectedMove.x < 0 || selectedMove.y < 0)
    {
        gameOver_ = true;
        winnerName_ = "None";
        statusMessage_ = "AI found no legal move";
        return;
    }

    if (!playMove(selectedMove))
        statusMessage_ = "AI selected an invalid move";
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
    drawWinLine();
}

void UIBoard::drawWinLine()
{
    if (winningLine_.cells.size() < 2)
        return;

    sf::Vector2f start(
        BOARD_OFFSET_X + winningLine_.cells.front().y * CELL_SIZE,
        BOARD_OFFSET_Y + winningLine_.cells.front().x * CELL_SIZE);
    sf::Vector2f end(
        BOARD_OFFSET_X + winningLine_.cells.back().y * CELL_SIZE,
        BOARD_OFFSET_Y + winningLine_.cells.back().x * CELL_SIZE);

    sf::Vector2f dir = end - start;
    float length = std::sqrt(dir.x * dir.x + dir.y * dir.y);

    sf::RectangleShape lineShape(sf::Vector2f(length, 6.f));
    lineShape.setOrigin({0.f, 3.f});
    lineShape.setPosition(start);

    float angle = std::atan2(dir.y, dir.x) * 180.f / 3.14159265f;
    lineShape.setRotation(sf::degrees(angle));
    lineShape.setFillColor(sf::Color(220, 30, 30, 230));

    window_.draw(lineShape);
}

void UIBoard::drawLines()
{
    for (int i = 0; i < 19; ++i)
    {
        float x = BOARD_OFFSET_X + i * CELL_SIZE;
        sf::Vertex line[] = {
            {sf::Vector2f(x, BOARD_OFFSET_Y), sf::Color::Black},
            {sf::Vector2f(x, BOARD_OFFSET_Y + BOARD_PIXEL_SIZE), sf::Color::Black}};
        window_.draw(line, 2, sf::PrimitiveType::Lines);
    }

    for (int i = 0; i < 19; ++i)
    {
        float y = BOARD_OFFSET_Y + i * CELL_SIZE;
        sf::Vertex line[] = {
            {sf::Vector2f(BOARD_OFFSET_X, y), sf::Color::Black},
            {sf::Vector2f(BOARD_OFFSET_X + BOARD_PIXEL_SIZE, y), sf::Color::Black}};
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

void UIBoard::drawGameOverOverlay()
{
    sf::RectangleShape backdrop(sf::Vector2f(640.f, 250.f));
    backdrop.setPosition({80.f, 285.f});
    backdrop.setFillColor(sf::Color(20, 23, 26, 180));
    backdrop.setOutlineThickness(4.f);
    backdrop.setOutlineColor(sf::Color(222, 184, 135));
    window_.draw(backdrop);

    sf::RectangleShape accent(sf::Vector2f(560.f, 4.f));
    accent.setPosition({120.f, 360.f});
    accent.setFillColor(sf::Color(222, 184, 135));
    window_.draw(accent);

    const std::string result = winnerName_ == "None"
                                   ? "DRAW GAME"
                                   : winnerName_ + " WINS!";

    drawText("GAME OVER", 270.f, 310.f, 34);
    drawText(result, 260.f, 385.f, 28);
    drawText(statusMessage_, 145.f, 435.f, 18);
    drawText("Use [ New Game ] or [ Mode ] to continue", 130.f, 485.f, 16);
}


void UIBoard::drawPanel()
{
    sf::RectangleShape panel(sf::Vector2f(300.f, 860.f));
    panel.setPosition({800.f, 0.f});
    panel.setFillColor(sf::Color(38, 43, 48));
    window_.draw(panel);

    std::string turn = currentTurn_ == Stone::BLACK ? "Black" : "White";
    std::string mode = mode_ == GameMode::HumanVsHuman ? "Human vs Human" : "Human vs AI";
    
    std::string ruleStr = "Standard";
    if (ruleVariant_ == RuleVariant::PRO) ruleStr = "Pro";
    else if (ruleVariant_ == RuleVariant::SWAP) ruleStr = "Swap";
    else if (ruleVariant_ == RuleVariant::SWAP2) ruleStr = "Swap2";
    else if (ruleVariant_ == RuleVariant::RENJU) ruleStr = "Renju";

    auto suggestion_move = requestSuggestion();

    std::string diffStr = "Medium";
    if (difficulty_ == Difficulty::EASY) diffStr = "Easy";
    else if (difficulty_ == Difficulty::HARD) diffStr = "Hard";

    drawText("Gomoku", 830.f, 40.f, 32);

    if (gameOver_)
    {
        drawText("GAME OVER", 830.f, 95.f, 22);
        drawText("Winner: " + getWinnerName(), 830.f, 125.f, 18);
    }
    else
    {
        drawText("Turn: " + turn, 830.f, 100.f, 22);
    }

    drawText("Mode: " + mode, 830.f, 150.f, 18);
    drawText("[ Rule: " + ruleStr + " ]", 830.f, 185.f, 18);
    drawText("Status: " + statusMessage_, 830.f, 215.f, 14);

    drawText("Captures", 830.f, 250.f, 22);
    drawText("Black: " + std::to_string(blackCaptures_), 830.f, 285.f, 18);
    drawText("White: " + std::to_string(whiteCaptures_), 830.f, 315.f, 18);

    drawText("AI time: " + std::to_string(static_cast<int>(lastAiTimeMs_)) + " ms", 830.f, 370.f, 18);
    if (suggestion_move.has_value()) {
        drawText(
            "Suggestion: " + std::to_string(suggestion_move->position.x) +
            ", " + std::to_string(suggestion_move->position.y),
            830.f,
            400.f,
            18
        );
    }

    drawText("[ New Game ]", 830.f, 460.f, 20);
    drawText("[ Suggest Move ]", 830.f, 500.f, 20);
    drawText("[ Mode ]", 830.f, 540.f, 20);
    

    drawText("[ Difficulty: " + diffStr + " ]", 830.f, 610.f, 18);
    // Render Swap/Swap2 options inside side panel when active
    if ((ruleVariant_ == RuleVariant::SWAP2 || ruleVariant_ == RuleVariant::SWAP)
        && !openingManager_.isOpeningFinished())
    {
        Swap2Phase phase = openingManager_.getSwap2Phase();

        if (phase == Swap2Phase::PLAYER2_CHOICE)
        {
            drawText("-- Player 2: Choose --", 830.f, 580.f, 16);
            drawText("> Stay White", 830.f, 605.f, 16);
            drawText("> Swap to Black", 830.f, 635.f, 16);
            if (ruleVariant_ == RuleVariant::SWAP2)
            {
                drawText("> Place 2 More Stones", 830.f, 665.f, 16);
            }
        }
        else if (phase == Swap2Phase::PLAYER1_CHOICE)
        {
            // FIX: previously never drawn, matching the missing click handler.
            drawText("-- Player 1: Choose --", 830.f, 580.f, 16);
            drawText("> Play Black", 830.f, 605.f, 16);
            drawText("> Play White", 830.f, 635.f, 16);
        }
    }
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
        resetGameState();
        return true;
    }

    if (isInsideRect(mousePos, humanVsAiButton))
    {
        mode_ = GameMode::HumanVsAI;
        ai_.setDifficulty(difficulty_); 

        if (ruleVariant_ == RuleVariant::SWAP || ruleVariant_ == RuleVariant::SWAP2)
            ruleVariant_ = RuleVariant::STANDARD;

        screen_ = Screen::Game;
        resetGameState();
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
