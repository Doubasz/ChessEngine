#include "core/board.h"
#include "search/search.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

using namespace chess;
using namespace std::chrono_literals;

namespace {

Board boardFrom(const char* fen) {
    Board board;
    EXPECT_TRUE(board.setFen(fen)) << fen;
    return board;
}

SearchResult searchDepth(const char* fen, int depth) {
    Searcher searcher;
    SearchLimits limits;
    limits.depth = depth;
    return searcher.search(boardFrom(fen), limits);
}

bool isLegal(Board board, Move m) {
    MoveList moves;
    board.generateLegalMoves(moves);
    for (Move legal : moves) {
        if (legal == m) return true;
    }
    return false;
}

}  // namespace


// ---- Finding the right move ------------------------------------------------------

TEST(Search, FindsBackRankMate) {
    SearchResult result = searchDepth("6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", 3);
    EXPECT_EQ(result.bestMove.toUci(), "a1a8");
    EXPECT_EQ(result.score, MateScore - 1);
    EXPECT_EQ(mateInMoves(result.score), 1);
}

TEST(Search, FindsMateForBlack) {
    SearchResult result = searchDepth("r5k1/8/8/8/8/8/5PPP/6K1 b - - 0 1", 3);
    EXPECT_EQ(result.bestMove.toUci(), "a8a1");
    EXPECT_EQ(mateInMoves(result.score), 1);
}

TEST(Search, FindsMateInTwo) {
    //  Rook ladder: a2a7, b1b7 and b1g1 all mate next move, nothing mates at once
    SearchResult result = searchDepth("7k/8/8/8/8/8/R7/1R5K w - - 0 1", 4);
    EXPECT_EQ(result.score, MateScore - 3);
    EXPECT_EQ(mateInMoves(result.score), 2);

    std::string best = result.bestMove.toUci();
    EXPECT_TRUE(best == "a2a7" || best == "b1b7" || best == "b1g1") << best;
}

TEST(Search, PrefersMateOverStalemate) {
    //  Most queen moves stalemate the black king, g1g7 mates
    SearchResult result = searchDepth("7k/8/5K2/8/8/8/8/6Q1 w - - 0 1", 3);
    EXPECT_EQ(result.bestMove.toUci(), "g1g7");
}

TEST(Search, TakesAHangingQueen) {
    SearchResult result = searchDepth("4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1", 2);
    EXPECT_EQ(result.bestMove.toUci(), "d1d5");
    EXPECT_GT(result.score, 400);
}

TEST(Search, DoesNotTakeADefendedPawnWithTheQueen) {
    //  Qxd5 wins a pawn but cxd5 wins the queen, quiescence has to see the recapture even at depth 1
    SearchResult result = searchDepth("4k3/8/2p5/3p4/8/8/8/3QK3 w - - 0 1", 1);
    EXPECT_NE(result.bestMove.toUci(), "d1d5");
}


// ---- Positions without a move ----------------------------------------------------

TEST(Search, CheckmatedPositionHasNoMove) {
    SearchResult result = searchDepth("R5k1/5ppp/8/8/8/8/8/6K1 b - - 0 1", 3);
    EXPECT_TRUE(result.bestMove.isNull());
    EXPECT_EQ(result.score, -MateScore);
}

TEST(Search, StalematedPositionScoresZero) {
    SearchResult result = searchDepth("7k/8/6Q1/5K2/8/8/8/8 b - - 0 1", 3);
    EXPECT_TRUE(result.bestMove.isNull());
    EXPECT_EQ(result.score, 0);
}


// ---- Repetitions -----------------------------------------------------------------

namespace {

//  King and queen against king, after the queen and the black king went there and back once
Board queenShuffled(int plies) {
    Board board = boardFrom("4k3/8/8/8/8/8/8/Q3K3 w - - 0 1");
    const char* moves[] = {"a1a2", "e8d8", "a2a1", "d8e8"};
    for (int i = 0; i < plies; i++) board.makeMove(board.moveFromUci(moves[i]));
    return board;
}

}  // namespace

TEST(Search, LosingSideRepeatsThePosition) {
    //  Every black move loses, except going back to e8: the starting position again, a draw
    Searcher searcher;
    SearchLimits limits;
    limits.depth = 3;
    SearchResult result = searcher.search(queenShuffled(3), limits);

    EXPECT_EQ(result.bestMove.toUci(), "d8e8");
    EXPECT_EQ(result.score, 0);
}

TEST(Search, WinningSideAvoidsRepeatingThePosition) {
    Searcher searcher;
    SearchLimits limits;
    limits.depth = 3;
    SearchResult result = searcher.search(queenShuffled(4), limits);

    EXPECT_NE(result.bestMove.toUci(), "a1a2");  // the position after it was already played
    EXPECT_GT(result.score, 0);
}


// ---- Limits ----------------------------------------------------------------------

TEST(Search, ReportsEveryDepthUpToTheLimit) {
    Searcher searcher;
    SearchLimits limits;
    limits.depth = 4;

    std::vector<int> depths;
    SearchResult result = searcher.search(Board(), limits, [&](const SearchResult& r) { depths.push_back(r.depth); });

    EXPECT_EQ(depths, (std::vector<int>{1, 2, 3, 4}));
    EXPECT_EQ(result.depth, 4);
    EXPECT_TRUE(isLegal(Board(), result.bestMove));
    EXPECT_GT(result.nodes, 0u);
}

TEST(Search, StopsAtTheTimeLimit) {
    Searcher searcher;
    SearchLimits limits;
    limits.time = 100ms;

    auto start = std::chrono::steady_clock::now();
    SearchResult result = searcher.search(Board(), limits);
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, 1s);
    EXPECT_GE(result.depth, 1);
    EXPECT_TRUE(isLegal(Board(), result.bestMove));
}

TEST(Search, StopsWhenAskedFromAnotherThread) {
    Searcher searcher;
    SearchResult result;

    auto start = std::chrono::steady_clock::now();
    std::thread worker([&] { result = searcher.search(Board(), SearchLimits()); });  // no limit at all
    std::this_thread::sleep_for(100ms);
    searcher.stop();
    worker.join();
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, 2s);
    EXPECT_GE(result.depth, 1);
    EXPECT_TRUE(isLegal(Board(), result.bestMove));
}
