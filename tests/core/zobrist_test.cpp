#include "core/board.h"

#include <gtest/gtest.h>

#include <initializer_list>

using namespace chess;

namespace {

Board boardFrom(const char* fen) {
    Board board;
    EXPECT_TRUE(board.setFen(fen)) << fen;
    return board;
}

void play(Board& board, std::initializer_list<const char*> moves) {
    for (const char* uci : moves) {
        Move m = board.moveFromUci(uci);
        ASSERT_FALSE(m.isNull()) << uci << " in " << board.fen();
        board.makeMove(m);
    }
}

//  setFen computes the hash from scratch: after every move of the tree, the hash makeMove
//  updated must be the one of the same position loaded from its FEN
void expectHashMatchesFen(Board& board, int depth) {
    ASSERT_EQ(board.hash(), boardFrom(board.fen().c_str()).hash()) << board.fen();
    if (depth == 0) return;

    MoveList moves;
    board.generateLegalMoves(moves);
    for (Move m : moves) {
        uint64_t before = board.hash();
        board.makeMove(m);
        expectHashMatchesFen(board, depth - 1);
        board.unmakeMove();
        ASSERT_EQ(board.hash(), before) << "after taking back " << m.toUci();
    }
}

}  // namespace


// ---- Hash ---------------------------------------------------------------------

//  The perft positions cover castling, rook captures that remove castling rights, en passant
//  and promotions
TEST(Zobrist, IncrementalHashMatchesHashFromScratch) {
    const char* fens[] = {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
    };
    for (const char* fen : fens) {
        Board board = boardFrom(fen);
        expectHashMatchesFen(board, 3);
    }
}

TEST(Zobrist, EveryPartOfThePositionChangesTheHash) {
    uint64_t start = boardFrom("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1").hash();

    EXPECT_NE(start, boardFrom("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR b KQkq - 0 1").hash());
    EXPECT_NE(start, boardFrom("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w Qkq - 0 1").hash());
    EXPECT_NE(start, boardFrom("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w - - 0 1").hash());
    EXPECT_NE(start, boardFrom("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKB1R w KQkq - 0 1").hash());
    EXPECT_NE(start, boardFrom("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKQBNR w kq - 0 1").hash());
}

TEST(Zobrist, MoveCountersAreNotPartOfTheHash) {
    EXPECT_EQ(boardFrom("4k3/8/8/8/8/8/8/4K2R w K - 0 1").hash(),
              boardFrom("4k3/8/8/8/8/8/8/4K2R w K - 37 60").hash());
}

TEST(Zobrist, SamePositionReachedByDifferentMovesHasTheSameHash) {
    Board a, b;
    play(a, {"e2e4", "e7e5", "g1f3"});
    play(b, {"g1f3", "e7e5", "e2e4"});  // ends with a double push, but no pawn can take en passant

    EXPECT_EQ(a.hash(), b.hash());
}

TEST(Zobrist, EnPassantCountsOnlyWhenACaptureIsPossible) {
    //  No black pawn next to e4
    EXPECT_EQ(boardFrom("4k3/8/8/8/4P3/8/8/4K3 b - e3 0 1").hash(),
              boardFrom("4k3/8/8/8/4P3/8/8/4K3 b - - 0 1").hash());

    //  The d4 pawn can take on e3
    EXPECT_NE(boardFrom("4k3/8/8/8/3pP3/8/8/4K3 b - e3 0 1").hash(),
              boardFrom("4k3/8/8/8/3pP3/8/8/4K3 b - - 0 1").hash());
}


// ---- Repetitions --------------------------------------------------------------

TEST(Repetition, CountsEarlierOccurrences) {
    Board board;
    EXPECT_EQ(board.repetitions(), 0);

    play(board, {"g1f3", "g8f6", "f3g1", "f6g8"});
    EXPECT_EQ(board.repetitions(), 1);
    EXPECT_EQ(board.status(), GameStatus::Ongoing);

    play(board, {"g1f3"});
    EXPECT_EQ(board.repetitions(), 1);  // the position after the first g1f3

    play(board, {"g8f6", "f3g1", "f6g8"});
    EXPECT_EQ(board.repetitions(), 2);
    EXPECT_EQ(board.status(), GameStatus::ThreefoldRepetition);

    board.unmakeMove();
    EXPECT_EQ(board.repetitions(), 1);
    EXPECT_EQ(board.status(), GameStatus::Ongoing);
}

TEST(Repetition, LostCastlingRightsMakeADifferentPosition) {
    Board board = boardFrom("r3k3/8/8/8/8/8/8/R3K3 w Qq - 0 1");

    play(board, {"e1e2", "e8e7", "e2e1", "e7e8"});  // same pieces, but no castling anymore
    EXPECT_EQ(board.repetitions(), 0);

    play(board, {"e1e2", "e8e7", "e2e1", "e7e8"});
    EXPECT_EQ(board.repetitions(), 1);
}

TEST(Repetition, PositionsBeforeAPawnMoveCannotRepeat) {
    Board board;
    play(board, {"g1f3", "g8f6", "f3g1", "f6g8", "e2e3", "e7e6"});
    EXPECT_EQ(board.repetitions(), 0);

    play(board, {"g1f3", "g8f6", "f3g1", "f6g8"});
    EXPECT_EQ(board.repetitions(), 1);
}
