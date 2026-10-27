#include "core/types.h"

namespace chess {

namespace {
//  Indexed by Piece value, the gaps (7, 8) are unused codes
constexpr std::string_view PieceChars = ".PNBRQK..pnbrqk";
}

char pieceToChar(Piece p) {
    return p < PieceChars.size() ? PieceChars[p] : '.';
}

Piece pieceFromChar(char c) {
    size_t index = PieceChars.find(c);
    return index == std::string_view::npos ? NoPiece : Piece(index);
}

std::string squareToString(Square s) {
    if (!isOnBoard(s)) return "-";
    return {char('a' + fileOf(s)), char('1' + rankOf(s))};
}

Square squareFromString(std::string_view s) {
    if (s.size() != 2) return NoSquare;
    int file = s[0] - 'a';
    int rank = s[1] - '1';
    if (file < 0 || file > 7 || rank < 0 || rank > 7) return NoSquare;
    return makeSquare(file, rank);
}

}  // namespace chess
