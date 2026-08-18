#pragma once

#include <gomoku/Board.hpp>
#include <gomoku/Rules.hpp>

#include <optional>
#include <vector>
#include <bit>
#include <cstdint>

class MinimaxAI
{
public:
    explicit MinimaxAI(int maxDepth = 3);

    Position findBestMove(
        const Board &board,
        Stone aiColor,
        RuleVariant variant,
        int blackCaptures,
        int whiteCaptures) const;

private:
    struct SearchState
    {
        Board board;

        Bitboard black;
        Bitboard white;

        int blackCaptures;
        int whiteCaptures;
    };

    static constexpr int WIN_SCORE = 100'000'000;
    static constexpr int INFINITY_SCORE = 1'000'000'000;

    static constexpr int MAX_CANDIDATE_MOVES = 8;

    static constexpr int CANDIDATE_RADIUS = 2;

    static constexpr int WINDOW_MARGIN = 1;

    int maxDepth_;

    int minimax(
        const SearchState &state,
        int depth,
        int alpha,
        int beta,
        Stone currentPlayer,
        Stone aiColor,
        RuleVariant variant) const;

    std::optional<int> getTerminalScore(
        const SearchState &state,
        Stone aiColor,
        RuleVariant variant,
        int depth) const;

    int evaluate(
        const SearchState &state,
        Stone aiColor) const;

    int evaluateColor(
        const Bitboard &ours,
        const Bitboard &opponent) const;

    int evaluateWindow(
        int stoneCount,
        int openEnds) const;

    std::vector<Position> generateMoves(
        const Board &board,
        Stone player,
        RuleVariant variant) const;

    bool hasNearbyStone(
        const Board &board,
        const Position &position) const;

    int scoreMoveForOrdering(
        const Board &board,
        const Position &move,
        Stone player,
        RuleVariant variant) const;

    static Stone opponentOf(Stone color);
};