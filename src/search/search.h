#pragma once

#include "core/board.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>

namespace chess {

//  Scores are in centipawns from the side to move's point of view. A forced mate scores
//  MateScore minus the number of plies to reach it, so a quicker mate scores higher.
constexpr int Infinity = 32000;
constexpr int MateScore = 31000;
constexpr int MaxPly = 128;

constexpr bool isMateScore(int score) {
    return score >= MateScore - MaxPly || score <= -(MateScore - MaxPly);
}

//  Full moves until mate, positive if the side to move mates, negative if it gets mated
constexpr int mateInMoves(int score) {
    return score > 0 ? (MateScore - score + 1) / 2 : -(MateScore + score + 1) / 2;
}

struct SearchLimits {
    int depth = MaxPly;  // in plies
    std::chrono::milliseconds time{0};  // 0 means no time limit
};

struct SearchResult {
    Move bestMove;  // null only if the position has no legal move
    int score = 0;
    int depth = 0;  // of the last fully searched iteration
    uint64_t nodes = 0;
    std::chrono::milliseconds elapsed{0};
};

//  Finds the best move with an alpha-beta search, deepened one ply at a time (iterative
//  deepening) until the depth or time limit is reached or stop() is called. When it stops in the
//  middle of an iteration the result of the previous, complete one is returned. Depth 1 always
//  completes, so there is a move to play whenever one exists.
//
//  Not done yet: repetition draws (needs position hashing), transposition table.
class Searcher {
public:
    //  onIteration, if given, is called after every completed depth with the result so far
    using IterationCallback = std::function<void(const SearchResult&)>;

    SearchResult search(const Board& board, const SearchLimits& limits, const IterationCallback& onIteration = nullptr);

    //  Asks a running search to stop as soon as possible. Safe to call from another thread.
    void stop() { stopRequested_ = true; }

    //  Nodes searched so far by the running search, updated every few thousand nodes.
    //  Safe to call from another thread, to show progress while the search runs.
    uint64_t nodesSearched() const { return nodesShared_.load(std::memory_order_relaxed); }

private:
    int searchRoot(int depth, Move& bestMove);
    int negamax(int depth, int alpha, int beta, int ply);
    int quiescence(int alpha, int beta, int ply);

    void orderMoves(const MoveList& moves, std::array<Move, MoveList::Capacity>& ordered, int ply, Move first) const;
    bool isCapture(Move m) const;
    std::chrono::milliseconds elapsed() const;
    bool shouldStop();

    Board board_;
    uint64_t nodes_ = 0;
    int completedDepth_ = 0;

    std::atomic<bool> stopRequested_{false};
    std::atomic<uint64_t> nodesShared_{0};
    bool stopped_ = false;  // set once the search must unwind, by stop() or by the time limit
    std::chrono::steady_clock::time_point start_;
    std::chrono::milliseconds timeLimit_{0};

    //  Two quiet moves per ply that caused a beta cutoff, tried right after the captures
    std::array<std::array<Move, 2>, MaxPly> killers_{};
};

}  // namespace chess
