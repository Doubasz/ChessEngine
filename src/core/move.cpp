#include "core/move.h"

namespace chess {

std::string Move::toUci() const {
    if (isNull()) return "0000";

    std::string uci = squareToString(from()) + squareToString(to());

    //  The promotion letter is always lowercase in UCI, whatever the color
    if (promotion() != NoPieceType) uci += pieceToChar(makePiece(Black, promotion()));

    return uci;
}

}  // namespace chess
