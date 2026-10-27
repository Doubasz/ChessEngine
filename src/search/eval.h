#pragma once

#include "core/board.h"

namespace chess {

//  Material values in centipawns (a pawn is 100). The king has no material value, it is never captured.
int pieceValue(PieceType type);

//  Static evaluation of the position in centipawns, from the side to move's point of view:
//  positive means the side to move is better. Material plus a bonus or malus for where each
//  piece stands (piece-square tables), the king switches to its endgame table once the queens
//  or most of the pieces are gone.
int evaluate(const Board& board);

}  // namespace chess
