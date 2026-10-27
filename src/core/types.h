#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace chess {

// ---- Colors and pieces --------------------------------------------------------
//
//  These are plain enums inside the namespace rather than enum class: they are used as
//  array indexes and in bit tricks all over the move generator, and enum class would
//  need a cast at every one of those places.

enum Color : uint8_t { White, Black };

constexpr Color operator~(Color c) { return Color(c ^ 1); }

enum PieceType : uint8_t { NoPieceType, Pawn, Knight, Bishop, Rook, Queen, King };

//  A piece packs its color in bit 3 and its type in bits 0-2, so 0 means an empty square
enum Piece : uint8_t {
    NoPiece = 0,
    WhitePawn = 1, WhiteKnight, WhiteBishop, WhiteRook, WhiteQueen, WhiteKing,
    BlackPawn = 9, BlackKnight, BlackBishop, BlackRook, BlackQueen, BlackKing,
};

constexpr Piece makePiece(Color c, PieceType t) { return Piece((c << 3) | t); }
constexpr PieceType typeOf(Piece p) { return PieceType(p & 7); }
constexpr Color colorOf(Piece p) { return Color(p >> 3); }

//  FEN letters: uppercase for white, lowercase for black
char pieceToChar(Piece p);  // '.' for NoPiece
Piece pieceFromChar(char c);  // NoPiece if c is not a piece letter


// ---- Squares: the 0x88 layout ------------------------------------------------
//
//  The board is stored 16 files wide: files 0-7 are the real board, files 8-15 are padding.
//
//      index = rank * 16 + file        a1 = 0x00, h1 = 0x07, a2 = 0x10, h8 = 0x77
//
//  Moving one square is a fixed offset (+1 east, +16 north, +17 north-east...). When a move
//  runs off the board the result has bit 0x08 set (file went past h) or bit 0x80 set (rank went
//  past 8, or the index went negative), so a single test, (sq & 0x88) != 0, catches every case.

enum Square : int {
    A1 = 0x00, B1, C1, D1, E1, F1, G1, H1,
    A2 = 0x10, B2, C2, D2, E2, F2, G2, H2,
    A3 = 0x20, B3, C3, D3, E3, F3, G3, H3,
    A4 = 0x30, B4, C4, D4, E4, F4, G4, H4,
    A5 = 0x40, B5, C5, D5, E5, F5, G5, H5,
    A6 = 0x50, B6, C6, D6, E6, F6, G6, H6,
    A7 = 0x60, B7, C7, D7, E7, F7, G7, H7,
    A8 = 0x70, B8, C8, D8, E8, F8, G8, H8,
    NoSquare = -1,
};

constexpr int BoardArraySize = 128;

constexpr Square operator+(Square s, int offset) { return Square(int(s) + offset); }
constexpr Square operator-(Square s, int offset) { return Square(int(s) - offset); }

constexpr bool isOnBoard(Square s) { return (s & 0x88) == 0; }
constexpr int fileOf(Square s) { return s & 7; }
constexpr int rankOf(Square s) { return s >> 4; }
constexpr Square makeSquare(int file, int rank) { return Square(rank * 16 + file); }

std::string squareToString(Square s);  // "e4", "-" for NoSquare
Square squareFromString(std::string_view s);  // NoSquare if s is not a square name


// ---- Castling rights ----------------------------------------------------------

enum CastlingRights : uint8_t {
    NoCastling = 0,
    WhiteKingside = 1,
    WhiteQueenside = 2,
    BlackKingside = 4,
    BlackQueenside = 8,
    AllCastling = 15,
};

}  // namespace chess
