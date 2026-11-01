#include "uci/uci.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace chess;
using namespace std::chrono_literals;

namespace {

std::vector<std::string> lines(const std::ostringstream& out) {
    std::istringstream in(out.str());
    std::vector<std::string> result;
    for (std::string line; std::getline(in, line);) result.push_back(line);
    return result;
}

bool hasLineStartingWith(const std::ostringstream& out, const std::string& prefix) {
    for (const std::string& line : lines(out)) {
        if (line.rfind(prefix, 0) == 0) return true;
    }
    return false;
}

std::string lastLine(const std::ostringstream& out) {
    auto all = lines(out);
    return all.empty() ? "" : all.back();
}

}  // namespace


// ---- Commands -----------------------------------------------------------------

TEST(Uci, AnswersTheHandshake) {
    std::ostringstream out;
    UciEngine engine(out);

    EXPECT_TRUE(engine.handle("uci"));
    EXPECT_TRUE(hasLineStartingWith(out, "id name "));
    EXPECT_TRUE(hasLineStartingWith(out, "id author "));
    EXPECT_TRUE(hasLineStartingWith(out, "option name Hash type spin"));
    EXPECT_EQ(lastLine(out), "uciok");

    engine.handle("isready");
    EXPECT_EQ(lastLine(out), "readyok");
}

TEST(Uci, IgnoresUnknownCommandsAndStopsOnQuit) {
    std::ostringstream out;
    UciEngine engine(out);

    EXPECT_TRUE(engine.handle("xyzzy"));
    EXPECT_TRUE(engine.handle(""));
    EXPECT_TRUE(engine.handle("debug on"));
    EXPECT_EQ(out.str(), "");
    EXPECT_FALSE(engine.handle("quit"));
}

TEST(Uci, SearchesToTheGivenDepth) {
    std::ostringstream out;
    UciEngine engine(out);

    engine.handle("position startpos");
    engine.handle("go depth 3");
    engine.waitForSearch();

    EXPECT_TRUE(hasLineStartingWith(out, "info depth 1 score cp "));
    EXPECT_TRUE(hasLineStartingWith(out, "info depth 3 score cp "));
    EXPECT_FALSE(hasLineStartingWith(out, "info depth 4"));

    std::string bestmove = lastLine(out);
    ASSERT_EQ(bestmove.rfind("bestmove ", 0), 0u) << bestmove;
    EXPECT_FALSE(Board().moveFromUci(bestmove.substr(9)).isNull()) << bestmove;
}

TEST(Uci, PlaysTheMovesAfterThePosition) {
    std::ostringstream out;
    UciEngine engine(out);

    //  Scholar's mate is on the board after these moves
    engine.handle("position startpos moves e2e4 e7e5 d1h5 b8c6 f1c4 g8f6");
    engine.handle("go depth 2");
    engine.waitForSearch();

    EXPECT_TRUE(hasLineStartingWith(out, "info depth 1 score mate 1 "));
    EXPECT_EQ(lastLine(out), "bestmove h5f7");
}

TEST(Uci, AcceptsAFenWithMoves) {
    std::ostringstream out;
    UciEngine engine(out);

    //  After Kh1 h8, the black king walked into a back-rank mate: Ra8#
    engine.handle("position fen 6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1 moves g1h1 g8h8");
    engine.handle("go depth 2");
    engine.waitForSearch();

    EXPECT_EQ(lastLine(out), "bestmove a1a8");
}

TEST(Uci, ReportsAnInvalidPosition) {
    std::ostringstream out;
    UciEngine engine(out);

    engine.handle("position fen not a fen at all");
    EXPECT_EQ(lastLine(out), "info string invalid fen: not a fen at all");

    engine.handle("position startpos moves e2e4 e2e4");
    EXPECT_EQ(lastLine(out), "info string illegal move: e2e4");
}

TEST(Uci, NoLegalMoveAnswersTheNullMove) {
    std::ostringstream out;
    UciEngine engine(out);

    engine.handle("position fen R5k1/5ppp/8/8/8/8/8/6K1 b - - 0 1");
    engine.handle("go depth 3");
    engine.waitForSearch();

    EXPECT_EQ(lastLine(out), "bestmove 0000");
}

TEST(Uci, AcceptsTheHashOptionAndANewGame) {
    std::ostringstream out;
    UciEngine engine(out);

    engine.handle("setoption name Hash value 4");
    engine.handle("ucinewgame");
    engine.handle("position startpos");
    engine.handle("go depth 2");
    engine.waitForSearch();

    EXPECT_EQ(lastLine(out).rfind("bestmove ", 0), 0u);
}


// ---- Stopping -----------------------------------------------------------------

TEST(Uci, InfiniteSearchRunsUntilStop) {
    std::ostringstream out;
    UciEngine engine(out);

    engine.handle("position startpos");
    engine.handle("go infinite");
    std::this_thread::sleep_for(200ms);

    engine.handle("isready");  // answered during the search
    engine.handle("stop");

    auto all = lines(out);
    EXPECT_NE(std::find(all.begin(), all.end(), "readyok"), all.end());
    EXPECT_EQ(lastLine(out).rfind("bestmove ", 0), 0u);
}

TEST(Uci, InfiniteSearchWaitsForStopEvenAfterAMate) {
    std::ostringstream out;
    UciEngine engine(out);

    engine.handle("position startpos moves e2e4 e7e5 d1h5 b8c6 f1c4 g8f6");
    engine.handle("go infinite");
    std::this_thread::sleep_for(100ms);  // the mate is found at depth 1, long before that
    EXPECT_FALSE(hasLineStartingWith(out, "bestmove"));

    engine.handle("stop");
    EXPECT_EQ(lastLine(out), "bestmove h5f7");
}

TEST(Uci, NewSearchStopsTheRunningOne) {
    std::ostringstream out;
    UciEngine engine(out);

    engine.handle("position startpos");
    engine.handle("go infinite");
    engine.handle("go depth 1");
    engine.waitForSearch();

    int bestmoves = 0;
    for (const std::string& line : lines(out)) {
        if (line.rfind("bestmove ", 0) == 0) bestmoves++;
    }
    EXPECT_EQ(bestmoves, 2);
}

TEST(Uci, MovetimeIsRespected) {
    std::ostringstream out;
    UciEngine engine(out);

    auto start = std::chrono::steady_clock::now();
    engine.handle("position startpos");
    engine.handle("go movetime 300");
    engine.waitForSearch();
    auto elapsed = std::chrono::steady_clock::now() - start;

    EXPECT_LT(elapsed, 1s);
    EXPECT_EQ(lastLine(out).rfind("bestmove ", 0), 0u);
}


// ---- Time management ----------------------------------------------------------

TEST(UciLimits, DepthAndMovetime) {
    SearchLimits depth = limitsFromGo("depth 6", White);
    EXPECT_EQ(depth.depth, 6);
    EXPECT_EQ(depth.time.count(), 0);

    SearchLimits movetime = limitsFromGo("movetime 1000", White);
    EXPECT_EQ(movetime.time.count(), 950);  // minus the overhead
}

TEST(UciLimits, InfiniteHasNoLimit) {
    SearchLimits limits = limitsFromGo("infinite", White);
    EXPECT_EQ(limits.depth, MaxPly);
    EXPECT_EQ(limits.time.count(), 0);
}

TEST(UciLimits, ClockOfTheSideToMove) {
    const std::string go = "wtime 60000 btime 30000 winc 1000 binc 0";
    EXPECT_EQ(limitsFromGo(go, White).time.count(), 60000 / 30 + 750);
    EXPECT_EQ(limitsFromGo(go, Black).time.count(), 30000 / 30);
}

TEST(UciLimits, MovesToGo) {
    EXPECT_EQ(limitsFromGo("wtime 10000 btime 10000 movestogo 10", White).time.count(), 1000);
    EXPECT_EQ(limitsFromGo("wtime 10000 btime 10000 movestogo 1", White).time.count(), 10000 - 50);
}

TEST(UciLimits, NeverMoreThanTheClock) {
    //  A big increment but almost no time left
    EXPECT_EQ(limitsFromGo("wtime 200 btime 200 winc 5000 binc 5000", White).time.count(), 150);
    EXPECT_EQ(limitsFromGo("wtime 20 btime 20", White).time.count(), 1);
}
