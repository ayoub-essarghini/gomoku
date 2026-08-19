#include <gomoku/Minmax.hpp>

#include <algorithm>
#include <cstddef>
#include <array>
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

    constexpr int CANDIDATE_RADIUS = 2;

    constexpr std::size_t TRANSPOSITION_TABLE_SIZE = 1U << 20;

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

    void collectStonePositions(
        const Board &board,
        std::vector<Position> &stones)
    {
        stones.clear();
        stones.reserve(Board::SIZE);

        for (int row = 0; row < Board::HEIGHT; ++row)
        {
            for (int column = 0; column < Board::WIDTH; ++column)
            {
                Position position{row, column};

                if (board.hasStone(position, Stone::BLACK) ||
                    board.hasStone(position, Stone::WHITE))
                {
                    stones.push_back(position);
                }
            }
        }
    }

    std::vector<std::array<int, 4>> buildWindows(
        const Board &board)
    {
        std::vector<Position> stones;
        collectStonePositions(board, stones);

        std::vector<std::array<int, 4>> windows;
        std::vector<bool> visited(stones.size(), false);

        for (std::size_t start = 0; start < stones.size(); ++start)
        {
            if (visited[start])
                continue;

            int minRow = stones[start].x;
            int maxRow = stones[start].x;
            int minColumn = stones[start].y;
            int maxColumn = stones[start].y;

            std::vector<std::size_t> component{start};
            visited[start] = true;

            for (std::size_t index = 0; index < component.size(); ++index)
            {
                const Position current = stones[component[index]];

                minRow = std::min(minRow, current.x);
                maxRow = std::max(maxRow, current.x);
                minColumn = std::min(minColumn, current.y);
                maxColumn = std::max(maxColumn, current.y);

                for (std::size_t other = 0; other < stones.size(); ++other)
                {
                    if (visited[other])
                        continue;

                    if (std::abs(current.x - stones[other].x) <= CANDIDATE_RADIUS &&
                        std::abs(current.y - stones[other].y) <= CANDIDATE_RADIUS)
                    {
                        visited[other] = true;
                        component.push_back(other);
                    }
                }
            }

            constexpr int windowMargin = 1;
            minRow = std::max(0, minRow - windowMargin);
            maxRow = std::min(Board::HEIGHT - 1, maxRow + windowMargin);
            minColumn = std::max(0, minColumn - windowMargin);
            maxColumn = std::min(Board::WIDTH - 1, maxColumn + windowMargin);

            windows.push_back({minRow, maxRow, minColumn, maxColumn});
        }

        return windows;
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
        whiteCaptures
    };

    std::vector<Position> moves = generateMoves(board, aiColor, variant);

    if (moves.empty())
        return Position{-1, -1};

    transpositionTable.clear();

    Position bestMove = moves.front();
    int bestScore = -INFINITY_SCORE;
    int alpha = -INFINITY_SCORE;
    const int beta = INFINITY_SCORE;

    for (const Position &move : moves)
    {
        SearchState childState = rootState;

        if (!childState.board.placeStone(move, aiColor))
            continue;

        const int capturedPairs = RuleChecker::applyCaptures(
            childState.board,
            move,
            aiColor);

        if (aiColor == Stone::BLACK)
            childState.blackCaptures += capturedPairs;
        else
            childState.whiteCaptures += capturedPairs;

        const int score = minimax(
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
    // The public/private MinimaxAI interface stays unchanged. The recursive
    // implementation lives in this lambda so no new class members are needed.
    auto search = [this, aiColor, variant](auto &&self,
                         const SearchState &node,
                         int remainingDepth,
                         int localAlpha,
                         int localBeta,
                         Stone player) -> int
    {
        const int originalAlpha = localAlpha;
        const int originalBeta = localBeta;

        const std::uint64_t hash = initialHash(node.board);
        const std::uint64_t key = positionKey(
            hash,
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

            if (!child.board.placeStone(move, player))
                continue;

            const int captures = RuleChecker::applyCaptures(
                child.board,
                move,
                player);

            if (player == Stone::BLACK)
                child.blackCaptures += captures;
            else
                child.whiteCaptures += captures;

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
        const Bitboard blackBitboard =
            RuleChecker::extractBitboard(
                state.board,
                Stone::BLACK);

        const Bitboard whiteBitboard =
            RuleChecker::extractBitboard(
                state.board,
                Stone::WHITE);

        const bool blackHasFive =
            RuleChecker::hasFiveInARow(blackBitboard);

        const bool whiteHasFive =
            RuleChecker::hasFiveInARow(whiteBitboard);

        const bool blackHasOverline =
            RuleChecker::hasOverline(blackBitboard);

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

    // Evaluate both players in one board pass. The previous implementation
    // scanned all windows once for AI and then scanned them again for the
    // opponent.
    int aiScore = 0;
    int opponentScore = 0;

    for (const auto &direction : DIRECTIONS)
    {
        const int dr = direction[0];
        const int dc = direction[1];

        for (int row = 0; row < Board::HEIGHT; ++row)
        {
            for (int column = 0; column < Board::WIDTH; ++column)
            {
                const int endRow = row + 4 * dr;
                const int endColumn = column + 4 * dc;

                if (!isInsideBoard(endRow, endColumn))
                    continue;

                int aiCount = 0;
                int opponentCount = 0;

                for (int step = 0; step < 5; ++step)
                {
                    Position position{
                        row + step * dr,
                        column + step * dc};

                    if (state.board.hasStone(position, aiColor))
                        ++aiCount;
                    else if (state.board.hasStone(position, opponent))
                        ++opponentCount;
                }

                if (opponentCount == 0)
                {
                    int openEnds = 0;
                    Position before{row - dr, column - dc};
                    Position after{endRow + dr, endColumn + dc};

                    if (isInsideBoard(before.x, before.y) &&
                        state.board.isEmpty(before))
                    {
                        ++openEnds;
                    }

                    if (isInsideBoard(after.x, after.y) &&
                        state.board.isEmpty(after))
                    {
                        ++openEnds;
                    }

                    aiScore += lineScore(aiCount, openEnds);
                }

                if (aiCount == 0)
                {
                    int openEnds = 0;
                    Position before{row - dr, column - dc};
                    Position after{endRow + dr, endColumn + dc};

                    if (isInsideBoard(before.x, before.y) &&
                        state.board.isEmpty(before))
                    {
                        ++openEnds;
                    }

                    if (isInsideBoard(after.x, after.y) &&
                        state.board.isEmpty(after))
                    {
                        ++openEnds;
                    }

                    opponentScore += lineScore(opponentCount, openEnds);
                }
            }
        }
    }

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

    // Avoid duplicates when two component windows overlap.
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

                // Keep a small bounded list instead of sorting hundreds of
                // moves at every search node.
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