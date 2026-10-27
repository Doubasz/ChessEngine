#pragma once

#include "core/move.h"
#include "core/types.h"

#include <array>
#include <string>
#include <string_view>

namespace chess {

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

private:
    struct EmptyTag {};
    explicit Board(EmptyTag);  // no pieces at all, used as the starting point of setFen

    std::array<Piece, BoardArraySize> squares_{};
    Color sideToMove_ = White;
    int castlingRights_ = NoCastling;
    Square enPassant_ = NoSquare;
    int halfmoveClock_ = 0;
    int fullmoveNumber_ = 1;

    //  Kept up to date by makeMove so check detection does not have to search for the king
    std::array<Square, 2> kings_{NoSquare, NoSquare};
};

}  // namespace chess
