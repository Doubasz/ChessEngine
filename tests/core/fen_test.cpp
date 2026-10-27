#include "core/board.h"

#include <gtest/gtest.h>

using namespace chess;

TEST(Fen, DefaultBoardIsTheStartingPosition) {
    Board board;
    EXPECT_EQ(board.fen(), Board::StartFen);
    EXPECT_EQ(board.sideToMove(), White);
    EXPECT_EQ(board.castlingRights(), AllCastling);
    EXPECT_EQ(board.pieceAt(E1), WhiteKing);
    EXPECT_EQ(board.pieceAt(D8), BlackQueen);
    EXPECT_EQ(board.pieceAt(E4), NoPiece);
    EXPECT_EQ(board.kingSquare(Black), E8);
}

//  Parsing then printing must give back the exact same string
TEST(Fen, RoundTrip) {
    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
        "rnbqkbnr/pppp1ppp/8/8/3Pp3/8/PPP1PPPP/RNBQKBNR b KQkq d3 0 3",
        "4k3/8/8/8/8/8/8/4K2R b K - 99 120",
    };
    for (const char* fen : fens) {
        Board board;
        ASSERT_TRUE(board.setFen(fen)) << fen;
        EXPECT_EQ(board.fen(), fen);
    }
}

TEST(Fen, CountersAreOptional) {
    Board board;
    ASSERT_TRUE(board.setFen("4k3/8/8/8/8/8/8/4K3 w - -"));
    EXPECT_EQ(board.halfmoveClock(), 0);
    EXPECT_EQ(board.fullmoveNumber(), 1);
}

TEST(Fen, ReadsEnPassantAndCounters) {
    Board board;
    ASSERT_TRUE(board.setFen("rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 4 3"));
    EXPECT_EQ(board.enPassantSquare(), D6);
    EXPECT_EQ(board.halfmoveClock(), 4);
    EXPECT_EQ(board.fullmoveNumber(), 3);
}

//  A castling right without the king and rook on their squares is dropped,
//  so the move generator never castles with a rook that is not there
TEST(Fen, DropsCastlingRightsWithoutKingAndRook) {
    Board board;
    ASSERT_TRUE(board.setFen("4k3/8/8/8/8/8/8/R3K3 w KQkq - 0 1"));
    EXPECT_EQ(board.castlingRights(), WhiteQueenside);
    EXPECT_EQ(board.fen(), "4k3/8/8/8/8/8/8/R3K3 w Q - 0 1");
}

TEST(Fen, RejectsInvalidFenAndKeepsThePosition) {
    const char* invalid[] = {
        "",
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP w KQkq - 0 1",  // 7 ranks
        "rnbqkbnr/pppppppp/9/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  // 9 empty squares
        "rnbqkbnr/ppppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",  // 9 pieces on a rank
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNX w KQkq - 0 1",  // unknown piece
        "rnbq1bnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQ - 0 1",  // no black king
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBKKBNR w kq - 0 1",  // two white kings
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR x KQkq - 0 1",  // bad side to move
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkx - 0 1",  // bad castling letter
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq e4 0 1",  // en passant on the wrong rank
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - x 1",  // bad halfmove clock
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 0",  // fullmove starts at 1
    };
    for (const char* fen : invalid) {
        Board board;
        EXPECT_FALSE(board.setFen(fen)) << fen;
        EXPECT_EQ(board.fen(), Board::StartFen) << "board changed after rejecting: " << fen;
    }
}

TEST(Squares, OffBoardDetection) {
    EXPECT_TRUE(isOnBoard(A1));
    EXPECT_TRUE(isOnBoard(H8));
    EXPECT_FALSE(isOnBoard(H1 + 1));  // past the h-file
    EXPECT_FALSE(isOnBoard(A8 + 16));  // past rank 8
    EXPECT_FALSE(isOnBoard(A1 - 1));  // negative index
    EXPECT_FALSE(isOnBoard(B1 - 33));  // knight jump off the bottom
}

TEST(Squares, NamesRoundTrip) {
    EXPECT_EQ(squareToString(E4), "e4");
    EXPECT_EQ(squareFromString("h8"), H8);
    EXPECT_EQ(squareFromString("i1"), NoSquare);
    EXPECT_EQ(squareToString(NoSquare), "-");
}

TEST(Moves, UciNotation) {
    EXPECT_EQ(Move(E2, E4, MoveKind::DoublePush).toUci(), "e2e4");
    EXPECT_EQ(Move(E1, G1, MoveKind::Castling).toUci(), "e1g1");
    EXPECT_EQ(Move(D7, C8, MoveKind::Normal, Knight).toUci(), "d7c8n");
    EXPECT_EQ(Move().toUci(), "0000");
}
