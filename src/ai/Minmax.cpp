#include <gomoku/Minmax.hpp>

#include <algorithm>
#include <cstddef>
#include <array>
#include <chrono>
#include <cstdint>
#include <limits>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace
{
    constexpr int DIRECTIONS[4][2] = {
        {0, 1},
        {1, 0},
        {1, 1},
        {1, -1}};

    constexpr std::size_t TRANSPOSITION_TABLE_SIZE = 1U << 20;

    struct TimeUp
    {
    };

    enum class BoundType : std::uint8_t
    {
        Exact,
        Lower,
        Upper
    };

    struct TTEntry
    {
        int depth = -1;
        int score = 0;
        BoundType bound = BoundType::Exact;
    };

    struct ScoredMove
    {
        Position position;
        int score;
    };

    std::unordered_map<std::uint64_t, TTEntry> transpositionTable;

    std::uint64_t splitMix64(std::uint64_t value)
    {
        value += 0x9E3779B97F4A7C15ULL;
        value = (value ^ (value >> 30U)) * 0xBF58476D1CE4E5B9ULL;
        value = (value ^ (value >> 27U)) * 0x94D049BB133111EBULL;
        return value ^ (value >> 31U);
    }

    const std::array<std::array<std::uint64_t, 2>,
                     Board::HEIGHT * Board::WIDTH> &zobristKeys()
    {
        static const auto keys = []
        {
            std::array<std::array<std::uint64_t, 2>,
                       Board::HEIGHT * Board::WIDTH> result{};
            std::uint64_t seed = 0x123456789ABCDEF0ULL;

            for (auto &entry : result)
            {
                entry[0] = splitMix64(seed);
                seed = splitMix64(seed);
                entry[1] = splitMix64(seed);
                seed = splitMix64(seed);
            }

            return result;
        }();

        return keys;
    }

    bool isInsideBoard(int row, int column)
    {
        return row >= 0 &&
               row < Board::HEIGHT &&
               column >= 0 &&
               column < Board::WIDTH;
    }

    std::uint64_t initialHash(const Board &board)
    {
        std::uint64_t hash = 0;
        const auto &keys = zobristKeys();

        for (int row = 0; row < Board::HEIGHT; ++row)
        {
            for (int column = 0; column < Board::WIDTH; ++column)
            {
                Position position{row, column};

                if (board.hasStone(position, Stone::BLACK))
                    hash ^= keys[row * Board::WIDTH + column][0];
                else if (board.hasStone(position, Stone::WHITE))
                    hash ^= keys[row * Board::WIDTH + column][1];
            }
        }

        return hash;
    }

    std::uint64_t positionKey(
        std::uint64_t boardHash,
        int blackCaptures,
        int whiteCaptures,
        Stone currentPlayer,
        RuleVariant variant)
    {
        std::uint64_t key = boardHash;
        key ^= splitMix64(static_cast<std::uint64_t>(blackCaptures) + 0x1000ULL);
        key ^= splitMix64(static_cast<std::uint64_t>(whiteCaptures) + 0x2000ULL);
        key ^= splitMix64(
            static_cast<std::uint64_t>(currentPlayer == Stone::BLACK ? 1 : 2) +
            0x3000ULL);
        key ^= splitMix64(
            static_cast<std::uint64_t>(static_cast<int>(variant)) + 0x4000ULL);
        return key;
    }

    int lineScore(int own, int openEnds)
    {
        if (openEnds == 0)
            return 0;

        switch (own)
        {
        case 5:
            return 10'000'000;
        case 4:
            return openEnds == 2 ? 1'000'000 : 100'000;
        case 3:
            return openEnds == 2 ? 50'000 : 5'000;
        case 2:
            return openEnds == 2 ? 1'000 : 100;
        case 1:
            return openEnds == 2 ? 10 : 1;
        default:
            return 0;
        }
    }

    int localMoveScore(
        const Board &board,
        const Position &move,
        Stone player)
    {
        int score = 0;
        const Stone opponent = player == Stone::BLACK
                                   ? Stone::WHITE
                                   : Stone::BLACK;

        for (const auto &direction : DIRECTIONS)
        {
            const int dr = direction[0];
            const int dc = direction[1];

            int own = 1;
            int openEnds = 0;

            for (int sign : {-1, 1})
            {
                int row = move.x + sign * dr;
                int column = move.y + sign * dc;

                while (isInsideBoard(row, column) &&
                       board.hasStone(Position{row, column}, player))
                {
                    ++own;
                    row += sign * dr;
                    column += sign * dc;
                }

                if (isInsideBoard(row, column) &&
                    board.isEmpty(Position{row, column}))
                {
                    ++openEnds;
                }
            }

            score += lineScore(own, openEnds);

            int opponentRun = 0;
            int opponentOpenEnds = 0;

            for (int sign : {-1, 1})
            {
                int row = move.x + sign * dr;
                int column = move.y + sign * dc;

                while (isInsideBoard(row, column) &&
                       board.hasStone(Position{row, column}, opponent))
                {
                    ++opponentRun;
                    row += sign * dr;
                    column += sign * dc;
                }

                if (isInsideBoard(row, column) &&
                    board.isEmpty(Position{row, column}))
                {
                    ++opponentOpenEnds;
                }
            }

            score += lineScore(opponentRun + 1, opponentOpenEnds) / 2;
        }

        const int centerRow = Board::HEIGHT / 2;
        const int centerColumn = Board::WIDTH / 2;
        const int centerDistance =
            std::abs(move.x - centerRow) +
            std::abs(move.y - centerColumn);

        score += Board::WIDTH - centerDistance;
        return score;
    }

    // Single bounding-box pass over all stones. Candidate cells are still
    // filtered by hasNearbyStone() afterwards, so a disjoint-component
    // clustering (previously O(stones^2) per node) buys nothing extra here.
    std::vector<std::array<int, 4>> buildWindows(
        const Board &board)
    {
        int minRow = Board::HEIGHT;
        int maxRow = -1;
        int minColumn = Board::WIDTH;
        int maxColumn = -1;

        for (int row = 0; row < Board::HEIGHT; ++row)
        {
            for (int column = 0; column < Board::WIDTH; ++column)
            {
                Position position{row, column};

                if (board.hasStone(position, Stone::BLACK) ||
                    board.hasStone(position, Stone::WHITE))
                {
                    minRow = std::min(minRow, row);
                    maxRow = std::max(maxRow, row);
                    minColumn = std::min(minColumn, column);
                    maxColumn = std::max(maxColumn, column);
                }
            }
        }

        if (maxRow < 0)
            return {};

        constexpr int windowMargin = 1;
        minRow = std::max(0, minRow - windowMargin);
        maxRow = std::min(Board::HEIGHT - 1, maxRow + windowMargin);
        minColumn = std::max(0, minColumn - windowMargin);
        maxColumn = std::min(Board::WIDTH - 1, maxColumn + windowMargin);

        return {{minRow, maxRow, minColumn, maxColumn}};
    }
}

MinimaxAI::MinimaxAI(int maxDepth)
    : maxDepth_(maxDepth > 0 ? maxDepth : 1)
{
    transpositionTable.reserve(TRANSPOSITION_TABLE_SIZE / 4);
}

Stone MinimaxAI::opponentOf(Stone color)
{
    return color == Stone::BLACK ? Stone::WHITE : Stone::BLACK;
}

bool MinimaxAI::applyMove(
    SearchState &state,
    const Position &move,
    Stone player) const
{
    if (!state.board.placeStone(move, player))
        return false;

    const int index = move.x * Board::WIDTH + move.y;
    const auto &keys = zobristKeys();

    state.hash ^= keys[static_cast<std::size_t>(index)][player == Stone::BLACK ? 0 : 1];

    if (player == Stone::BLACK)
        state.black.rows[move.x] |= (1U << move.y);
    else
        state.white.rows[move.x] |= (1U << move.y);

    const int capturedPairs = RuleChecker::applyCaptures(
        state.board,
        move,
        player);

    if (player == Stone::BLACK)
        state.blackCaptures += capturedPairs;
    else
        state.whiteCaptures += capturedPairs;

    if (capturedPairs > 0)
    {
        // Captured stones' exact positions aren't reported back, so resync
        // hash/bitboards from the board. Captures are rare, so this O(361)
        // fallback doesn't affect the hot (non-capturing) path.
        state.black = RuleChecker::extractBitboard(state.board, Stone::BLACK);
        state.white = RuleChecker::extractBitboard(state.board, Stone::WHITE);
        state.hash = initialHash(state.board);
    }

    return true;
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
        RuleChecker::extractBitboard(board, Stone::BLACK),
        RuleChecker::extractBitboard(board, Stone::WHITE),
        blackCaptures,
        whiteCaptures,
        initialHash(board)
    };

    std::vector<Position> moves = generateMoves(board, aiColor, variant);

    if (moves.empty())
        return Position{-1, -1};

    transpositionTable.clear();

    const auto deadline =
        std::chrono::steady_clock::now() +
        std::chrono::milliseconds(TIME_BUDGET_MS);

    Position confirmedBestMove = moves.front();

    for (int depth = 1; depth <= maxDepth_; ++depth)
    {
        Position bestMove = moves.front();
        int bestScore = -INFINITY_SCORE;
        int alpha = -INFINITY_SCORE;
        const int beta = INFINITY_SCORE;
        bool timedOut = false;

        try
        {
            for (const Position &move : moves)
            {
                SearchState childState = rootState;

                if (!applyMove(childState, move, aiColor))
                    continue;

                const int score = minimax(
                    childState,
                    depth - 1,
                    alpha,
                    beta,
                    opponentOf(aiColor),
                    aiColor,
                    variant,
                    deadline);

                if (score > bestScore)
                {
                    bestScore = score;
                    bestMove = move;
                }

                alpha = std::max(alpha, bestScore);
            }
        }
        catch (const TimeUp &)
        {
            timedOut = true;
        }

        if (timedOut)
            break;

        confirmedBestMove = bestMove;

        // 500ms is a ceiling, not a target: once a depth's search has found
        // a forced win/loss, deeper search can't change the decision, so
        // stop early instead of burning the rest of the budget.
        if (bestScore >= WIN_SCORE || bestScore <= -WIN_SCORE)
            break;

        // Search the previous depth's best move first next iteration so
        // alpha-beta cuts more of the tree.
        auto it = std::find_if(
            moves.begin(),
            moves.end(),
            [&bestMove](const Position &move)
            {
                return move.x == bestMove.x && move.y == bestMove.y;
            });

        if (it != moves.begin() && it != moves.end())
            std::rotate(moves.begin(), it, it + 1);

        if (std::chrono::steady_clock::now() >= deadline)
            break;
    }

    return confirmedBestMove;
}

int MinimaxAI::minimax(
    const SearchState &state,
    int depth,
    int alpha,
    int beta,
    Stone currentPlayer,
    Stone aiColor,
    RuleVariant variant,
    std::chrono::steady_clock::time_point deadline) const
{

    auto search = [this, aiColor, variant, deadline](auto &&self,
                         const SearchState &node,
                         int remainingDepth,
                         int localAlpha,
                         int localBeta,
                         Stone player) -> int
    {
        if (std::chrono::steady_clock::now() >= deadline)
            throw TimeUp{};

        const int originalAlpha = localAlpha;
        const int originalBeta = localBeta;

        const std::uint64_t key = positionKey(
            node.hash,
            node.blackCaptures,
            node.whiteCaptures,
            player,
            variant);

        auto tableIt = transpositionTable.find(key);

        if (tableIt != transpositionTable.end() &&
            tableIt->second.depth >= remainingDepth)
        {
            const TTEntry &entry = tableIt->second;

            if (entry.bound == BoundType::Exact)
                return entry.score;

            if (entry.bound == BoundType::Lower)
                localAlpha = std::max(localAlpha, entry.score);
            else
                localBeta = std::min(localBeta, entry.score);

            if (localAlpha >= localBeta)
                return entry.score;
        }

        const std::optional<int> terminal =
            getTerminalScore(node, aiColor, variant, remainingDepth);

        if (terminal.has_value())
            return terminal.value();

        if (remainingDepth == 0)
            return evaluate(node, aiColor);

        std::vector<Position> moves =
            generateMoves(node.board, player, variant);

        if (moves.empty())
            return evaluate(node, aiColor);

        const bool maximizing = player == aiColor;
        int best = maximizing ? -INFINITY_SCORE : INFINITY_SCORE;

        for (std::size_t i = 0; i < moves.size(); ++i)
        {
            const Position &move = moves[i];
            SearchState child = node;

            if (!this->applyMove(child, move, player))
                continue;

            int score;

            // Principal Variation Search. The first move receives a full
            // window; later moves get a null window and are re-searched only
            // when they appear capable of improving the bound.
            if (i == 0)
            {
                score = self(
                    self,
                    child,
                    remainingDepth - 1,
                    localAlpha,
                    localBeta,
                    opponentOf(player));
            }
            else if (maximizing)
            {
                score = self(
                    self,
                    child,
                    remainingDepth - 1,
                    localAlpha,
                    localAlpha + 1,
                    opponentOf(player));

                if (score > localAlpha && score < localBeta)
                {
                    score = self(
                        self,
                        child,
                        remainingDepth - 1,
                        localAlpha,
                        localBeta,
                        opponentOf(player));
                }
            }
            else
            {
                score = self(
                    self,
                    child,
                    remainingDepth - 1,
                    localBeta - 1,
                    localBeta,
                    opponentOf(player));

                if (score < localBeta && score > localAlpha)
                {
                    score = self(
                        self,
                        child,
                        remainingDepth - 1,
                        localAlpha,
                        localBeta,
                        opponentOf(player));
                }
            }

            if (maximizing)
            {
                best = std::max(best, score);
                localAlpha = std::max(localAlpha, best);
            }
            else
            {
                best = std::min(best, score);
                localBeta = std::min(localBeta, best);
            }

            if (localBeta <= localAlpha)
                break;
        }

        BoundType bound = BoundType::Exact;

        if (best <= originalAlpha)
            bound = BoundType::Upper;
        else if (best >= originalBeta)
            bound = BoundType::Lower;

        if (transpositionTable.size() >= TRANSPOSITION_TABLE_SIZE)
        {
            transpositionTable.clear();
            transpositionTable.reserve(TRANSPOSITION_TABLE_SIZE / 4);
        }

        transpositionTable[key] = TTEntry{
            remainingDepth,
            best,
            bound};

        return best;
    };

    return search(
        search,
        state,
        depth,
        alpha,
        beta,
        currentPlayer);
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
        const bool blackHasFive =
            RuleChecker::hasFiveInARow(state.black);

        const bool whiteHasFive =
            RuleChecker::hasFiveInARow(state.white);

        const bool blackHasOverline =
            RuleChecker::hasOverline(state.black);

        if (variant == RuleVariant::RENJU && blackHasOverline)
            winner = Stone::WHITE;
        else if (blackHasFive)
            winner = Stone::BLACK;
        else if (whiteHasFive)
            winner = Stone::WHITE;
    }

    if (winner == aiColor)
        return WIN_SCORE + depth;

    if (winner == opponentOf(aiColor))
        return -WIN_SCORE - depth;

    const int occupiedCells =
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
    const Stone opponent = opponentOf(aiColor);

    const Bitboard &aiBitboard =
        aiColor == Stone::BLACK ? state.black : state.white;

    const Bitboard &opponentBitboard =
        opponent == Stone::BLACK ? state.black : state.white;

    const int aiScore = evaluateColor(aiBitboard, opponentBitboard);
    const int opponentScore = evaluateColor(opponentBitboard, aiBitboard);

    const int aiCaptures = aiColor == Stone::BLACK
                               ? state.blackCaptures
                               : state.whiteCaptures;

    const int opponentCaptures = opponent == Stone::BLACK
                                     ? state.blackCaptures
                                     : state.whiteCaptures;

    constexpr int CAPTURE_PAIR_SCORE = 20'000;

    const int captureScore =
        (aiCaptures - opponentCaptures) * CAPTURE_PAIR_SCORE;

    const int defensiveOpponentScore =
        (opponentScore * 11) / 10;

    return aiScore - defensiveOpponentScore + captureScore;
}

int MinimaxAI::evaluateColor(
    const Bitboard& ours,
    const Bitboard& opponent) const
{
    int totalScore = 0;

    // horizontal
    for (int r = 0; r < Board::HEIGHT; ++r)
    {
        const uint32_t ourRow = ours.rows[r];
        const uint32_t opponentRow = opponent.rows[r];

        for (int c = 0;
             c <= Board::WIDTH - 5;
             ++c)
        {
            const uint32_t mask =
                0x1FU << c;

            const uint32_t ourWindow =
                ourRow & mask;

            const uint32_t opponentWindow =
                opponentRow & mask;

            if (opponentWindow != 0)
                continue;

            const int stoneCount =
                __builtin_popcount(ourWindow);

            if (stoneCount == 0)
                continue;

            int openEnds = 0;

            if (c > 0)
            {
                const uint32_t before =
                    1U << (c - 1);

                if (((ourRow | opponentRow) & before) == 0)
                    ++openEnds;
            }

            if (c + 5 < Board::WIDTH)
            {
                const uint32_t after =
                    1U << (c + 5);

                if (((ourRow | opponentRow) & after) == 0)
                    ++openEnds;
            }

            totalScore +=
                evaluateWindow(
                    stoneCount,
                    openEnds);
        }
    }

    // vertical
    for (int r = 0;
         r <= Board::HEIGHT - 5;
         ++r)
    {
        for (int c = 0;
             c < Board::WIDTH;
             ++c)
        {
            const uint32_t bit =
                1U << c;

            int stoneCount = 0;
            bool blocked = false;

            for (int i = 0; i < 5; ++i)
            {
                if (opponent.rows[r + i] & bit)
                {
                    blocked = true;
                    break;
                }

                if (ours.rows[r + i] & bit)
                    ++stoneCount;
            }

            if (blocked || stoneCount == 0)
                continue;

            int openEnds = 0;

            if (r > 0)
            {
                const uint32_t occupied =
                    ours.rows[r - 1] |
                    opponent.rows[r - 1];

                if ((occupied & bit) == 0)
                    ++openEnds;
            }

            if (r + 5 < Board::HEIGHT)
            {
                const uint32_t occupied =
                    ours.rows[r + 5] |
                    opponent.rows[r + 5];

                if ((occupied & bit) == 0)
                    ++openEnds;
            }

            totalScore +=
                evaluateWindow(
                    stoneCount,
                    openEnds);
        }
    }

    // diagonal down-right
    for (int r = 0;
         r <= Board::HEIGHT - 5;
         ++r)
    {
        for (int c = 0;
             c <= Board::WIDTH - 5;
             ++c)
        {
            int stoneCount = 0;
            bool blocked = false;

            for (int i = 0; i < 5; ++i)
            {
                const uint32_t bit =
                    1U << (c + i);

                if (opponent.rows[r + i] & bit)
                {
                    blocked = true;
                    break;
                }

                if (ours.rows[r + i] & bit)
                    ++stoneCount;
            }

            if (blocked || stoneCount == 0)
                continue;

            int openEnds = 0;

            if (r > 0 && c > 0)
            {
                const uint32_t bit =
                    1U << (c - 1);

                const uint32_t occupied =
                    ours.rows[r - 1] |
                    opponent.rows[r - 1];

                if ((occupied & bit) == 0)
                    ++openEnds;
            }

            if (r + 5 < Board::HEIGHT &&
                c + 5 < Board::WIDTH)
            {
                const uint32_t bit =
                    1U << (c + 5);

                const uint32_t occupied =
                    ours.rows[r + 5] |
                    opponent.rows[r + 5];

                if ((occupied & bit) == 0)
                    ++openEnds;
            }

            totalScore +=
                evaluateWindow(
                    stoneCount,
                    openEnds);
        }
    }

    // diagonal down-left
    for (int r = 0;
         r <= Board::HEIGHT - 5;
         ++r)
    {
        for (int c = 4;
             c < Board::WIDTH;
             ++c)
        {
            int stoneCount = 0;
            bool blocked = false;

            for (int i = 0; i < 5; ++i)
            {
                const uint32_t bit =
                    1U << (c - i);

                if (opponent.rows[r + i] & bit)
                {
                    blocked = true;
                    break;
                }

                if (ours.rows[r + i] & bit)
                    ++stoneCount;
            }

            if (blocked || stoneCount == 0)
                continue;

            int openEnds = 0;

            if (r > 0 &&
                c + 1 < Board::WIDTH)
            {
                const uint32_t bit =
                    1U << (c + 1);

                const uint32_t occupied =
                    ours.rows[r - 1] |
                    opponent.rows[r - 1];

                if ((occupied & bit) == 0)
                    ++openEnds;
            }

            if (r + 5 < Board::HEIGHT &&
                c - 5 >= 0)
            {
                const uint32_t bit =
                    1U << (c - 5);

                const uint32_t occupied =
                    ours.rows[r + 5] |
                    opponent.rows[r + 5];

                if ((occupied & bit) == 0)
                    ++openEnds;
            }

            totalScore +=
                evaluateWindow(
                    stoneCount,
                    openEnds);
        }
    }

    return totalScore;
}

int MinimaxAI::evaluateWindow(
    int stoneCount,
    int openEnds) const
{
    return lineScore(stoneCount, openEnds);
}

std::vector<Position> MinimaxAI::generateMoves(
    const Board &board,
    Stone player,
    RuleVariant variant) const
{
    const int occupiedCells =
        board.countStones(Stone::BLACK) +
        board.countStones(Stone::WHITE);

    if (occupiedCells == 0)
        return {Position{Board::HEIGHT / 2, Board::WIDTH / 2}};

    const int moveNumber = occupiedCells + 1;
    std::vector<std::array<int, 4>> windows = buildWindows(board);

    std::array<bool, Board::HEIGHT * Board::WIDTH> seen{};
    std::vector<ScoredMove> scoredMoves;
    scoredMoves.reserve(MAX_CANDIDATE_MOVES + 8);

    for (const auto &window : windows)
    {
        for (int row = window[0]; row <= window[1]; ++row)
        {
            for (int column = window[2]; column <= window[3]; ++column)
            {
                const int index = row * Board::WIDTH + column;

                if (seen[index])
                    continue;

                seen[index] = true;

                Position position{row, column};

                if (!board.isEmpty(position))
                    continue;

                if (!hasNearbyStone(board, position))
                    continue;

                if (variant == RuleVariant::PRO &&
                    !RuleChecker::isValidProMove(moveNumber, position))
                {
                    continue;
                }

                if (variant == RuleVariant::RENJU &&
                    player == Stone::BLACK &&
                    RuleChecker::isRenjuFoul(board, position))
                {
                    continue;
                }

                const int score =
                    localMoveScore(board, position, player);

                
                scoredMoves.push_back({position, score});
            }
        }
    }

    if (scoredMoves.empty())
        return {};

    const std::size_t limit =
        std::min<std::size_t>(
            MAX_CANDIDATE_MOVES,
            scoredMoves.size());

    if (scoredMoves.size() > limit)
    {
        std::nth_element(
            scoredMoves.begin(),
            scoredMoves.begin() + static_cast<std::ptrdiff_t>(limit),
            scoredMoves.end(),
            [](const ScoredMove &left, const ScoredMove &right)
            {
                return left.score > right.score;
            });

        scoredMoves.erase(
            scoredMoves.begin() + static_cast<std::ptrdiff_t>(limit),
            scoredMoves.end());
    }

    std::sort(
        scoredMoves.begin(),
        scoredMoves.end(),
        [](const ScoredMove &left, const ScoredMove &right)
        {
            if (left.score != right.score)
                return left.score > right.score;

            if (left.position.x != right.position.x)
                return left.position.x < right.position.x;

            return left.position.y < right.position.y;
        });

    std::vector<Position> moves;
    moves.reserve(scoredMoves.size());

    for (const ScoredMove &move : scoredMoves)
        moves.push_back(move.position);

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
            if (rowOffset == 0 && columnOffset == 0)
                continue;

            Position neighbor{
                position.x + rowOffset,
                position.y + columnOffset};

            if (!isInsideBoard(neighbor.x, neighbor.y))
                continue;

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
    RuleVariant /*variant*/) const
{
    return localMoveScore(board, move, player);
}

void MinimaxAI::setDifficulty(Difficulty level)
{
    switch (level)
    {
        case Difficulty::EASY:
            maxDepth_ = 5;
            timeBudgetMs_ = 150;
            break;
        case Difficulty::MEDIUM:
            maxDepth_ = 10;
            timeBudgetMs_ = 500;
            break;
        case Difficulty::HARD:
            maxDepth_ = 11;
            timeBudgetMs_ = 1500;
            break;
    }
}