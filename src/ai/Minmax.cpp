#include <gomoku/Minmax.hpp>

#include <algorithm>
#include <cmath>
#include <utility>

namespace
{
    constexpr int DIRECTIONS[4][2] = {
        {0, 1},
        {1, 0},
        {1, 1},
        {1, -1}};
    
    struct Window{
        int minRow;
        int MaxRow;
        int minColums;
        int maxColums;
    };

    bool areClose(
        const Position& first,
        const Position& second,
    ){
        return  std::abs(first.y - second.y) <= CANDIDATE_RADIUS && 
                std::abs(first.x - second.x) ,= CANDIDATE_RADIUS;
    }

    bool windowsOverlap(
        const Window& first,
        const Window& second
    ){
        return !(first.MaxRow < second.minRow || 
                    second.MaxRow < first.MaxRow ||
                    first.maxColums < second.minColums ||
                    second.maxColums < first.minColums        
                );
    }

    Window mergeWindows(
        const Window& first,
        const Window& second
    ){
        return Window{
            std::min(first.minRow, second.minRow),
            std::max(first.maxRow, second.maxRow),
            std::min(first.minColums, second.minColums),
            std::max(first.maxColums, second.maxColums),
        };
    }
    
    Window buildWindowsForCluster(
        const std::vector<Position>& cluster
    ){
        int minRow = Board::HEIGHT - 1;
        int maxRow = 0;
        int minColums = Board::WIDTH - 1;
        int maxColums = 0;

        for (const Position& stone : cluster){
            minRow = std::min(minRow, stone.y);
            maxRow = std::max(maxRow, stone.y);
            minColums = std::min(minColums, stone.x);
            maxColums = std::max(maxColums, stone.x);
        }

        minRow -= WINDOWS_MARGIN;
        maxRow += WINDOWS_MARGIN;

        minColums -= WINDOWS_MARGIN;
        maxColums += WINDOWS_MARGIN;

        minRow = std::max(0, minRow);
        maxRow = std::min(Board::HEIGHT - 1, maxRow);

        minColums = std::max(0, minColums);
        maxColums = std::min(Board::WIDTH - 1, maxColums);

        return Window{
            minRow,
            MaxRow,
            minColums,
            maxColums
        };
    }

    bool isInsideBoard(int row, int column)
    {
        return row >= 0 &&
               row < Board::HEIGHT &&
               column >= 0 &&
               column < Board::WIDTH;
    }

}

MinimaxAI::MinimaxAI(int maxDepth)
    : maxDepth_(maxDepth > 0 ? maxDepth : 1)
{
}

Stone MinimaxAI::opponentOf(Stone color)
{
    if (color == Stone::BLACK)
        return Stone::WHITE;

    return Stone::BLACK;
}

Position MinimaxAI::findBestMove(
    const Board &board,
    Stone aiColor,
    RuleVariant variant,
    int blackCaptures,
    int whiteCaptures) const
{
    SearchState rootState{
        board,
        blackCaptures,
        whiteCaptures};

    std::vector<Position> moves =
        generateMoves(board, aiColor, variant);

    if (moves.empty())
        return Position{-1, -1};

    Position bestMove = moves.front();
    int bestScore = -INFINITY_SCORE;
    int alpha = -INFINITY_SCORE;
    const int beta = INFINITY_SCORE;

    for (const Position &move : moves)
    {
        SearchState childState = rootState;

        if (!childState.board.placeStone(move, aiColor))
            continue;

        int capturedPairs = RuleChecker::applyCaptures(
            childState.board,
            move,
            aiColor);

        if (aiColor == Stone::BLACK)
            childState.blackCaptures += capturedPairs;
        else
            childState.whiteCaptures += capturedPairs;

        int score = minimax(
            childState,
            maxDepth_ - 1,
            alpha,
            beta,
            opponentOf(aiColor),
            aiColor,
            variant);

        if (score > bestScore)
        {
            bestScore = score;
            bestMove = move;
        }

        alpha = std::max(alpha, bestScore);
    }

    return bestMove;
}

int MinimaxAI::minimax(
    const SearchState &state,
    int depth,
    int alpha,
    int beta,
    Stone currentPlayer,
    Stone aiColor,
    RuleVariant variant) const
{
    std::optional<int> terminalScore =
        getTerminalScore(state, aiColor, variant, depth);

    if (terminalScore.has_value())
        return terminalScore.value();

    if (depth == 0)
        return evaluate(state, aiColor);

    std::vector<Position> moves =
        generateMoves(state.board, currentPlayer, variant);

    if (moves.empty())
        return evaluate(state, aiColor);

    const bool maximizingPlayer = currentPlayer == aiColor;

    int bestScore = maximizingPlayer
                        ? -INFINITY_SCORE
                        : INFINITY_SCORE;

    for (const Position &move : moves)
    {
        SearchState childState = state;

        if (!childState.board.placeStone(move, currentPlayer))
            continue;

        int capturedPairs = RuleChecker::applyCaptures(
            childState.board,
            move,
            currentPlayer);

        if (currentPlayer == Stone::BLACK)
            childState.blackCaptures += capturedPairs;
        else
            childState.whiteCaptures += capturedPairs;

        int score = minimax(
            childState,
            depth - 1,
            alpha,
            beta,
            opponentOf(currentPlayer),
            aiColor,
            variant);

        if (maximizingPlayer)
        {
            bestScore = std::max(bestScore, score);
            alpha = std::max(alpha, bestScore);
        }
        else
        {
            bestScore = std::min(bestScore, score);
            beta = std::min(beta, bestScore);
        }

        if (beta <= alpha)
            break;
    }

    return bestScore;
}

std::optional<int> MinimaxAI::getTerminalScore(
    const SearchState &state,
    Stone aiColor,
    RuleVariant variant,
    int depth) const
{
    Stone winner = Stone::EMPTY;

    if (state.blackCaptures >= 5)
    {
        winner = Stone::BLACK;
    }
    else if (state.whiteCaptures >= 5)
    {
        winner = Stone::WHITE;
    }
    else
    {
        Bitboard blackBitboard =
            RuleChecker::extractBitboard(
                state.board,
                Stone::BLACK);

        Bitboard whiteBitboard =
            RuleChecker::extractBitboard(
                state.board,
                Stone::WHITE);

        bool blackHasFive =
            RuleChecker::hasFiveInARow(blackBitboard);

        bool whiteHasFive =
            RuleChecker::hasFiveInARow(whiteBitboard);

        bool blackHasOverline =
            RuleChecker::hasOverline(blackBitboard);

        if (variant == RuleVariant::RENJU &&
            blackHasOverline)
        {
            winner = Stone::WHITE;
        }
        else if (blackHasFive)
        {
            winner = Stone::BLACK;
        }
        else if (whiteHasFive)
        {
            winner = Stone::WHITE;
        }
    }

    if (winner == aiColor)
        return WIN_SCORE + depth;

    if (winner == opponentOf(aiColor))
        return -WIN_SCORE - depth;

    int occupiedCells =
        state.board.countStones(Stone::BLACK) +
        state.board.countStones(Stone::WHITE);

    if (occupiedCells == Board::SIZE)
        return 0;

    return std::nullopt;
}

int MinimaxAI::evaluate(
    const SearchState &state,
    Stone aiColor) const
{
    Stone opponent = opponentOf(aiColor);

    int aiPatternScore =
        evaluateColor(state.board, aiColor);

    int opponentPatternScore =
        evaluateColor(state.board, opponent);

    int aiCaptures = aiColor == Stone::BLACK
                         ? state.blackCaptures
                         : state.whiteCaptures;

    int opponentCaptures = opponent == Stone::BLACK
                               ? state.blackCaptures
                               : state.whiteCaptures;

    constexpr int CAPTURE_PAIR_SCORE = 20'000;

    int captureScore =
        (aiCaptures - opponentCaptures) *
        CAPTURE_PAIR_SCORE;

    int defensiveOpponentScore =
        (opponentPatternScore * 11) / 10;

    return aiPatternScore -
           defensiveOpponentScore +
           captureScore;
}

int MinimaxAI::evaluateColor(
    const Board &board,
    Stone color) const
{
    int totalScore = 0;
    Stone opponent = opponentOf(color);

    for (int row = 0; row < Board::HEIGHT; ++row)
    {
        for (int column = 0;
             column < Board::WIDTH;
             ++column)
        {
            for (const auto &direction : DIRECTIONS)
            {
                int rowDirection = direction[0];
                int columnDirection = direction[1];

                int endRow =
                    row + 4 * rowDirection;

                int endColumn =
                    column + 4 * columnDirection;

                if (!isInsideBoard(endRow, endColumn))
                    continue;

                int stoneCount = 0;
                int opponentCount = 0;

                for (int step = 0; step < 5; ++step)
                {
                    Position position{
                        row + step * rowDirection,
                        column + step * columnDirection};

                    if (board.hasStone(position, color))
                    {
                        ++stoneCount;
                    }
                    else if (board.hasStone(position, opponent))
                    {
                        ++opponentCount;
                    }
                }

                if (opponentCount != 0)
                    continue;

                int openEnds = 0;

                Position before{
                    row - rowDirection,
                    column - columnDirection};

                Position after{
                    endRow + rowDirection,
                    endColumn + columnDirection};

                if (isInsideBoard(before.x, before.y) &&
                    board.isEmpty(before))
                {
                    ++openEnds;
                }

                if (isInsideBoard(after.x, after.y) &&
                    board.isEmpty(after))
                {
                    ++openEnds;
                }

                totalScore +=
                    evaluateWindow(stoneCount, openEnds);
            }
        }
    }

    return totalScore;
}

int MinimaxAI::evaluateWindow(
    int stoneCount,
    int openEnds) const
{
    if (openEnds == 0)
        return 0;

    switch (stoneCount)
    {
    case 4:
        return openEnds == 2
                   ? 1'000'000
                   : 100'000;

    case 3:
        return openEnds == 2
                   ? 50'000
                   : 5'000;

    case 2:
        return openEnds == 2
                   ? 1'000
                   : 100;

    case 1:
        return openEnds == 2
                   ? 10
                   : 1;

    default:
        return 0;
    }
}

std::vector<Position> MinimaxAI::generateMoves(
    const Board &board,
    Stone player,
    RuleVariant variant) const
{
    int occupiedCells =
        board.countStones(Stone::BLACK) +
        board.countStones(Stone::WHITE);

    if (occupiedCells == 0)
        return {Position{Board::HEIGHT / 2, Board::WIDTH / 2}};

    struct ScoredMove
    {
        Position position;
        int score;
    };

    std::vector<ScoredMove> scoredMoves;
    int moveNumber = occupiedCells + 1;

    for (int row = 0; row < Board::HEIGHT; ++row)
    {
        for (int column = 0;
             column < Board::WIDTH;
             ++column)
        {
            Position position{row, column};

            if (!board.isEmpty(position))
                continue;

            if (!hasNearbyStone(board, position))
                continue;

            if (variant == RuleVariant::PRO &&
                !RuleChecker::isValidProMove(
                    moveNumber,
                    position))
            {
                continue;
            }

            if (variant == RuleVariant::RENJU &&
                player == Stone::BLACK &&
                RuleChecker::isRenjuFoul(
                    board,
                    position))
            {
                continue;
            }

            int orderingScore =
                scoreMoveForOrdering(
                    board,
                    position,
                    player,
                    variant);

            scoredMoves.push_back(
                ScoredMove{
                    position,
                    orderingScore});
        }
    }

    std::sort(
        scoredMoves.begin(),
        scoredMoves.end(),
        [](const ScoredMove &left,
           const ScoredMove &right)
        {
            if (left.score != right.score)
                return left.score > right.score;

            if (left.position.x != right.position.x)
                return left.position.x < right.position.x;

            return left.position.y < right.position.y;
        });

    if (scoredMoves.size() >
        static_cast<std::size_t>(MAX_CANDIDATE_MOVES))
    {
        scoredMoves.erase(
            scoredMoves.begin() + MAX_CANDIDATE_MOVES,
            scoredMoves.end());
    }

    std::vector<Position> moves;
    moves.reserve(scoredMoves.size());

    for (const ScoredMove &scoredMove : scoredMoves)
        moves.push_back(scoredMove.position);

    return moves;
}

bool MinimaxAI::hasNearbyStone(
    const Board &board,
    const Position &position) const
{
    for (int rowOffset = -CANDIDATE_RADIUS;
         rowOffset <= CANDIDATE_RADIUS;
         ++rowOffset)
    {
        for (int columnOffset = -CANDIDATE_RADIUS;
             columnOffset <= CANDIDATE_RADIUS;
             ++columnOffset)
        {
            if (rowOffset == 0 &&
                columnOffset == 0)
            {
                continue;
            }

            Position neighbor{
                position.x + rowOffset,
                position.y + columnOffset};

            if (board.hasStone(neighbor, Stone::BLACK) ||
                board.hasStone(neighbor, Stone::WHITE))
            {
                return true;
            }
        }
    }

    return false;
}

int MinimaxAI::scoreMoveForOrdering(
    const Board &board,
    const Position &move,
    Stone player,
    RuleVariant variant) const
{
    constexpr int IMMEDIATE_WIN_ORDER_SCORE = 50'000'000;
    constexpr int IMMEDIATE_BLOCK_ORDER_SCORE = 40'000'000;
    constexpr int CAPTURE_ORDER_SCORE = 5'000'000;
    constexpr int CAPTURE_BLOCK_ORDER_SCORE = 4'000'000;

    int orderingScore = 0;

    Board attackingBoard = board;

    if (!attackingBoard.placeStone(move, player))
        return -INFINITY_SCORE;

    int capturedPairs = RuleChecker::applyCaptures(
        attackingBoard,
        move,
        player);

    orderingScore += capturedPairs * CAPTURE_ORDER_SCORE;

    Bitboard attackingBitboard =
        RuleChecker::extractBitboard(
            attackingBoard,
            player);

    bool createsFive =
        RuleChecker::hasFiveInARow(
            attackingBitboard);

    bool createsIllegalBlackOverline =
        variant == RuleVariant::RENJU &&
        player == Stone::BLACK &&
        RuleChecker::hasOverline(
            attackingBitboard);

    if (createsFive &&
        !createsIllegalBlackOverline)
    {
        orderingScore +=
            IMMEDIATE_WIN_ORDER_SCORE;
    }

    Stone opponent = opponentOf(player);
    bool opponentMoveIsLegal = true;

    if (variant == RuleVariant::RENJU &&
        opponent == Stone::BLACK &&
        RuleChecker::isRenjuFoul(board, move))
    {
        opponentMoveIsLegal = false;
    }

    if (opponentMoveIsLegal)
    {
        Board defensiveBoard = board;

        if (defensiveBoard.placeStone(move, opponent))
        {
            int opponentCapturedPairs = RuleChecker::applyCaptures(
                defensiveBoard,
                move,
                opponent);

            orderingScore += opponentCapturedPairs * CAPTURE_BLOCK_ORDER_SCORE;

            Bitboard opponentBitboard =
                RuleChecker::extractBitboard(
                    defensiveBoard,
                    opponent);

            bool opponentCreatesFive =
                RuleChecker::hasFiveInARow(
                    opponentBitboard);

            bool opponentCreatesIllegalOverline =
                variant == RuleVariant::RENJU &&
                opponent == Stone::BLACK &&
                RuleChecker::hasOverline(
                    opponentBitboard);

            if (opponentCreatesFive &&
                !opponentCreatesIllegalOverline)
            {
                orderingScore +=
                    IMMEDIATE_BLOCK_ORDER_SCORE;
            }
        }
    }

    orderingScore +=
        evaluateColor(attackingBoard, player);

    int centerRow = Board::HEIGHT / 2;
    int centerColumn = Board::WIDTH / 2;

    int centerDistance =
        std::abs(move.x - centerRow) +
        std::abs(move.y - centerColumn);

    orderingScore +=
        Board::WIDTH - centerDistance;

    return orderingScore;
}
