#include "core/board.h"
#include "core/san.h"

#include <gtest/gtest.h>

#include <string>

using namespace chess;

namespace {

//  The SAN of the legal move written uci in the position fen
std::string san(const std::string& fen, const std::string& uci) {
    Board board;
    EXPECT_TRUE(board.setFen(fen)) << fen;
    Move m = board.moveFromUci(uci);
    EXPECT_FALSE(m.isNull()) << uci << " is not legal in " << fen;
    return toSan(board, m);
}

}  // namespace


TEST(San, PawnAndPieceMoves) {
    std::string start(Board::StartFen);
    EXPECT_EQ(san(start, "e2e4"), "e4");
    EXPECT_EQ(san(start, "e2e3"), "e3");
    EXPECT_EQ(san(start, "g1f3"), "Nf3");
    EXPECT_EQ(san(start, "b1c3"), "Nc3");
}

TEST(San, Captures) {
    std::string fen = "rnbqkbnr/ppp1pppp/8/3p4/4P3/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 2";
    EXPECT_EQ(san(fen, "e4d5"), "exd5");

    //  1.e4 d5 2.exd5 Qxd5
    EXPECT_EQ(san("rnbqkbnr/ppp1pppp/8/3P4/8/8/PPPP1PPP/RNBQKBNR b KQkq - 0 2", "d8d5"), "Qxd5");
}

TEST(San, EnPassantIsAPawnCapture) {
    std::string fen = "rnbqkbnr/ppp1p1pp/8/3pPp2/8/8/PPPP1PPP/RNBQKBNR w KQkq f6 0 3";
    EXPECT_EQ(san(fen, "e5f6"), "exf6");
}

TEST(San, Castling) {
    std::string fen = "r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1";
    EXPECT_EQ(san(fen, "e1g1"), "O-O");
    EXPECT_EQ(san(fen, "e1c1"), "O-O-O");

    std::string black = "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1";
    EXPECT_EQ(san(black, "e8g8"), "O-O");
    EXPECT_EQ(san(black, "e8c8"), "O-O-O");
}

TEST(San, Promotions) {
    std::string fen = "1n6/P7/8/8/8/8/8/1k5K w - - 0 1";
    EXPECT_EQ(san(fen, "a7a8q"), "a8=Q");
    EXPECT_EQ(san(fen, "a7a8n"), "a8=N");
    EXPECT_EQ(san(fen, "a7b8n"), "axb8=N");
    EXPECT_EQ(san(fen, "a7b8q"), "axb8=Q+");  // down the b file to the king on b1
}

TEST(San, CheckAndMate) {
    EXPECT_EQ(san("4k3/8/8/8/8/8/8/R3K3 w - - 0 1", "a1a8"), "Ra8+");

    //  Scholar's mate: 1.e4 e5 2.Bc4 Nc6 3.Qh5 Nf6 4.Qxf7#
    std::string scholar = "r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4";
    EXPECT_EQ(san(scholar, "h5f7"), "Qxf7#");
}

TEST(San, DisambiguationByFile) {
    std::string fen = "4k3/8/8/8/8/8/8/1N2KN2 w - - 0 1";
    EXPECT_EQ(san(fen, "b1d2"), "Nbd2");
    EXPECT_EQ(san(fen, "f1d2"), "Nfd2");
    EXPECT_EQ(san(fen, "b1c3"), "Nc3");  // only one knight reaches c3
}

TEST(San, DisambiguationByRank) {
    std::string fen = "4k3/8/8/R7/8/8/8/R3K3 w - - 0 1";
    EXPECT_EQ(san(fen, "a1a3"), "R1a3");
    EXPECT_EQ(san(fen, "a5a3"), "R5a3");
}

TEST(San, DisambiguationBySquare) {
    //  Three queens reach b2: a1 shares its file with a3 and its rank with c1
    std::string fen = "4k3/8/8/8/8/Q7/8/Q1Q1K3 w - - 0 1";
    EXPECT_EQ(san(fen, "a1b2"), "Qa1b2");
    EXPECT_EQ(san(fen, "a3b2"), "Q3b2");
    EXPECT_EQ(san(fen, "c1b2"), "Qcb2");
}

TEST(San, PinnedPieceDoesNotDisambiguate) {
    //  The knight on e2 also reaches c3, but it is pinned to its king by the rook on e8
    std::string fen = "4r1k1/8/8/8/8/8/4N3/1N2K3 w - - 0 1";
    EXPECT_EQ(san(fen, "b1c3"), "Nc3");
}

TEST(San, LeavesTheBoardUnchanged) {
    Board board;
    ASSERT_TRUE(board.setFen("r1bqkb1r/pppp1ppp/2n2n2/4p2Q/2B1P3/8/PPPP1PPP/RNB1K1NR w KQkq - 4 4"));
    std::string before = board.fen();

    toSan(board, board.moveFromUci("h5f7"));

    EXPECT_EQ(board.fen(), before);
    EXPECT_TRUE(board.lastMove().isNull());
}
