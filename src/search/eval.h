#pragma once

#include "core/board.h"

namespace chess {

//  Material values in centipawns (a pawn is 100). The king has no material value, it is never captured.
int pieceValue(PieceType type);

//  Static evaluation of the position in centipawns, from the side to move's point of view:
//  positive means the side to move is better. Material plus a bonus or malus for where each
//  piece stands (piece-square tables), the king switches to its endgame table once the queens
//  or most of the pieces are gone. Then the pawn structure of each side, see pawnStructure.
int evaluate(const Board& board);

//  The bonuses and penalties of c's pawns, in centipawns, part of evaluate:
//  - doubled pawns: each pawn more than one on a file is a penalty, they block each other
//  - isolated pawns: no pawn of the same color on the neighbor files to defend it, a penalty
//  - passed pawns: no enemy pawn in front of it on its file or the neighbor files to stop it,
//    a bonus that grows as it gets closer to promotion
int pawnStructure(const Board& board, Color c);

}  // namespace chess
