#include "core/board.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using namespace chess;

namespace {

Board boardFrom(const char* fen) {
    Board board;
    EXPECT_TRUE(board.setFen(fen)) << fen;
    return board;
}

std::vector<std::string> legalMovesUci(Board& board) {
    MoveList moves;
    board.generateLegalMoves(moves);

    std::vector<std::string> uci;
    for (Move m : moves) uci.push_back(m.toUci());
    std::sort(uci.begin(), uci.end());
    return uci;
}

bool hasMove(Board& board, const std::string& uci) {
    auto moves = legalMovesUci(board);
    return std::find(moves.begin(), moves.end(), uci) != moves.end();
}

}  // namespace


// ---- isSquareAttacked ---------------------------------------------------------

TEST(Attacks, PawnsAttackDiagonallyForward) {
    Board board = boardFrom("4k3/8/8/3p4/4P3/8/8/4K3 w - - 0 1");
    EXPECT_TRUE(board.isSquareAttacked(D5, White));  // e4 takes d5
    EXPECT_TRUE(board.isSquareAttacked(F5, White));
    EXPECT_FALSE(board.isSquareAttacked(E5, White));  // straight ahead is not an attack
    EXPECT_FALSE(board.isSquareAttacked(D3, White));  // nor backwards
    EXPECT_TRUE(board.isSquareAttacked(E4, Black));  // d5 attacks downwards
    EXPECT_FALSE(board.isSquareAttacked(E6, Black));
}

TEST(Attacks, KnightJumpsOverPieces) {
    Board board = boardFrom("4k3/8/8/8/8/2PPP3/2PNP3/4K3 w - - 0 1");
    EXPECT_TRUE(board.isSquareAttacked(B1, White));
    EXPECT_TRUE(board.isSquareAttacked(F1, White));
    EXPECT_TRUE(board.isSquareAttacked(C4, White));
    EXPECT_FALSE(board.isSquareAttacked(D5, White));  // nothing reaches d5 (d4 would be hit by the c3 and e3 pawns)
}

TEST(Attacks, SlidersStopAtTheFirstPiece) {
    Board board = boardFrom("4k3/8/8/8/R2p3r/8/8/4K3 w - - 0 1");
    EXPECT_TRUE(board.isSquareAttacked(D4, White));  // the rook reaches the pawn
    EXPECT_FALSE(board.isSquareAttacked(E4, White));  // but not past it
    EXPECT_TRUE(board.isSquareAttacked(A8, White));
    EXPECT_TRUE(board.isSquareAttacked(E4, Black));  // h4 rook from the other side
}

TEST(Attacks, QueenAttacksLikeRookAndBishop) {
    Board board = boardFrom("4k3/8/8/8/3Q4/8/8/4K3 w - - 0 1");
    EXPECT_TRUE(board.isSquareAttacked(D8, White));
    EXPECT_TRUE(board.isSquareAttacked(H8, White));
    EXPECT_TRUE(board.isSquareAttacked(A1, White));
    EXPECT_FALSE(board.isSquareAttacked(E6, White));  // knight distance, not a queen line
}


// ---- Move generation ----------------------------------------------------------

TEST(MoveGen, StartingPositionHasTwentyMoves) {
    Board board;
    EXPECT_EQ(legalMovesUci(board).size(), 20u);
}

TEST(MoveGen, PinnedPieceCannotLeaveThePin) {
    //  The e2 knight is pinned by the e8 rook against the e1 king
    Board board = boardFrom("4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1");
    for (const std::string& m : legalMovesUci(board)) EXPECT_NE(m.substr(0, 2), "e2") << m;
}

TEST(MoveGen, InCheckOnlyMovesThatAnswerTheCheck) {
    //  Rook check on the e-file: block with the bishop, take with the knight... or move the king
    Board board = boardFrom("4r1k1/8/8/8/8/2N5/3B4/4K3 w - - 0 1");
    EXPECT_TRUE(board.inCheck());
    EXPECT_TRUE(hasMove(board, "d2e3"));  // block
    EXPECT_TRUE(hasMove(board, "c3e4"));  // block
    EXPECT_TRUE(hasMove(board, "e1f2"));  // step aside
    EXPECT_FALSE(hasMove(board, "e1e2"));  // still on the file
    EXPECT_FALSE(hasMove(board, "c3b5"));  // ignores the check
}

TEST(MoveGen, PromotionsToAllFourPieces) {
    Board board = boardFrom("3r2k1/4P3/8/8/8/8/8/4K3 w - - 0 1");
    for (const char* m : {"e7e8q", "e7e8r", "e7e8b", "e7e8n", "e7d8q", "e7d8n"}) {
        EXPECT_TRUE(hasMove(board, m)) << m;
    }
    EXPECT_FALSE(hasMove(board, "e7e8"));
}

TEST(MoveGen, EnPassantCapture) {
    Board board = boardFrom("4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1");
    ASSERT_TRUE(hasMove(board, "e5d6"));

    board.makeMove(board.moveFromUci("e5d6"));
    EXPECT_EQ(board.pieceAt(D6), WhitePawn);
    EXPECT_EQ(board.pieceAt(D5), NoPiece);  // the captured pawn was beside, not on d6
}

TEST(MoveGen, EnPassantThatExposesTheKingIsIllegal) {
    //  b5xc6 would remove both pawns from the 5th rank and open the h5 rook onto the a5 king
    Board board = boardFrom("8/8/8/KPp4r/8/8/8/4k3 w - c6 0 1");
    EXPECT_FALSE(hasMove(board, "b5c6"));
}

TEST(MoveGen, CastlingBothSides) {
    Board board = boardFrom("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    EXPECT_TRUE(hasMove(board, "e1g1"));
    EXPECT_TRUE(hasMove(board, "e1c1"));
}

TEST(MoveGen, NoCastlingThroughOrOutOfCheck) {
    //  f1 attacked by the f8 rook: no kingside, queenside still fine
    Board through = boardFrom("4kr2/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    EXPECT_FALSE(hasMove(through, "e1g1"));
    EXPECT_TRUE(hasMove(through, "e1c1"));

    //  King in check: no castling at all
    Board inCheck = boardFrom("4r1k1/8/8/8/8/8/8/R3K2R w KQ - 0 1");
    EXPECT_FALSE(hasMove(inCheck, "e1g1"));
    EXPECT_FALSE(hasMove(inCheck, "e1c1"));
}

TEST(MoveGen, NoCastlingWithPiecesInBetween) {
    //  b1 is only crossed by the rook, it still has to be empty
    Board board = boardFrom("4k3/8/8/8/8/8/8/RN2K1nR w KQ - 0 1");
    EXPECT_FALSE(hasMove(board, "e1g1"));  // enemy knight on g1 blocks too
    EXPECT_FALSE(hasMove(board, "e1c1"));
}


// ---- makeMove / unmakeMove ----------------------------------------------------

TEST(MakeMove, CastlingMovesTheRook) {
    Board board = boardFrom("r3k2r/8/8/8/8/8/8/R3K2R w KQkq - 0 1");
    board.makeMove(board.moveFromUci("e1g1"));
    EXPECT_EQ(board.pieceAt(G1), WhiteKing);
    EXPECT_EQ(board.pieceAt(F1), WhiteRook);
    EXPECT_EQ(board.pieceAt(H1), NoPiece);
    EXPECT_EQ(board.castlingRights(), BlackKingside | BlackQueenside);
    EXPECT_EQ(board.kingSquare(White), G1);
}

TEST(MakeMove, CapturingARookRemovesThatCastlingRight) {
    Board board = boardFrom("r3k2r/8/8/8/8/8/6B1/R3K2R w KQkq - 0 1");
    board.makeMove(board.moveFromUci("g2a8"));
    EXPECT_EQ(board.castlingRights(), WhiteKingside | WhiteQueenside | BlackKingside);
}

TEST(MakeMove, CountersAndEnPassantSquare) {
    Board board;
    board.makeMove(board.moveFromUci("g1f3"));
    EXPECT_EQ(board.halfmoveClock(), 1);
    EXPECT_EQ(board.fullmoveNumber(), 1);

    board.makeMove(board.moveFromUci("e7e5"));
    EXPECT_EQ(board.halfmoveClock(), 0);  // pawn move resets it
    EXPECT_EQ(board.fullmoveNumber(), 2);  // black moved
    EXPECT_EQ(board.enPassantSquare(), E6);
}

//  The most important property: every move taken back leaves the exact same position,
//  checked on positions full of castling, en passant and promotions
TEST(MakeMove, UnmakeRestoresThePositionForEveryMove) {
    const char* fens[] = {
        Board::StartFen.data(),
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1",
        "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8",
        "rnbqkbnr/ppp1pppp/8/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3",
    };
    for (const char* fen : fens) {
        Board board = boardFrom(fen);
        MoveList moves;
        board.generateLegalMoves(moves);

        for (Move m : moves) {
            board.makeMove(m);
            board.unmakeMove();
            EXPECT_EQ(board.fen(), fen) << "after " << m.toUci();
            EXPECT_EQ(board.kingSquare(White), boardFrom(fen).kingSquare(White)) << m.toUci();
        }
    }
}


// ---- Game status --------------------------------------------------------------

TEST(Status, CheckmateStalemateAndFiftyMoves) {
    struct Case {
        const char* fen;
        GameStatus expected;
    };
    const Case cases[] = {
        {"rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", GameStatus::Ongoing},
        {"rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", GameStatus::Checkmate},
        {"R5k1/5ppp/8/8/8/8/8/6K1 b - - 0 1", GameStatus::Checkmate},
        {"7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", GameStatus::Stalemate},
        {"8/8/8/8/8/1k6/p7/K7 w - - 0 1", GameStatus::Stalemate},
        {"4k3/8/8/8/8/8/8/4K2R w - - 100 80", GameStatus::FiftyMoveRule},
        {"R5k1/5ppp/8/8/8/8/8/6K1 b - - 100 80", GameStatus::Checkmate},
    };
    for (const Case& c : cases) {
        Board board = boardFrom(c.fen);
        EXPECT_EQ(board.status(), c.expected) << c.fen;
    }
}
