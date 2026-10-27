#pragma once

#include "core/move.h"
#include "core/types.h"

#include <array>
#include <string>
#include <string_view>
#include <vector>

namespace chess {

enum class GameStatus {
    Ongoing,
    Checkmate,  // the side to move is mated
    Stalemate,
    FiftyMoveRule,
};

//  A chess position: where the pieces are, whose turn it is, castling rights,
//  en passant square and the two move counters.
//
//  Everything outside src/core (perft, GUI, the future search) goes through this public
//  interface only, so the 0x88 internals can later be swapped for bitboards without
//  touching the callers.
class Board {
public:
    static constexpr std::string_view StartFen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    Board();  // the starting position

    //  Loads a position. On an invalid FEN it returns false and leaves the board unchanged.
    //  The halfmove and fullmove fields are optional and default to "0 1".
    bool setFen(std::string_view fen);
    std::string fen() const;

    Piece pieceAt(Square s) const { return squares_[s]; }
    Color sideToMove() const { return sideToMove_; }
    int castlingRights() const { return castlingRights_; }
    Square enPassantSquare() const { return enPassant_; }  // NoSquare if none
    int halfmoveClock() const { return halfmoveClock_; }
    int fullmoveNumber() const { return fullmoveNumber_; }
    Square kingSquare(Color c) const { return kings_[c]; }

    bool isSquareAttacked(Square s, Color by) const;
    bool inCheck() const { return isSquareAttacked(kings_[sideToMove_], ~sideToMove_); }

    //  Fills moves with every legal move of the side to move. Not const: each candidate move
    //  is played and taken back to see if it leaves the king in check, the board ends up unchanged.
    void generateLegalMoves(MoveList& moves);

    //  The legal move written as uci (e2e4, e7e8q), or the null move if there is none
    Move moveFromUci(std::string_view uci);

    //  m must be legal in this position. unmakeMove takes back the last move made.
    void makeMove(Move m);
    void unmakeMove();

    GameStatus status();

private:
    //  Everything makeMove overwrites and unmakeMove cannot work out from the move itself
    struct Undo {
        Move move;
        Piece captured;
        int castlingRights;
        Square enPassant;
        int halfmoveClock;
    };

    struct EmptyTag {};
    explicit Board(EmptyTag);  // no pieces at all, used as the starting point of setFen

    void addIfLegal(MoveList& moves, Move m);
    void generatePawnMoves(MoveList& moves, Square from);
    void generateCastling(MoveList& moves);

    std::array<Piece, BoardArraySize> squares_{};
    Color sideToMove_ = White;
    int castlingRights_ = NoCastling;
    Square enPassant_ = NoSquare;
    int halfmoveClock_ = 0;
    int fullmoveNumber_ = 1;

    //  Kept up to date by makeMove so check detection does not have to search for the king
    std::array<Square, 2> kings_{NoSquare, NoSquare};

    std::vector<Undo> history_;
};

}  // namespace chess
