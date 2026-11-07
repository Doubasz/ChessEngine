#pragma once

#include "core/board.h"
#include "core/move.h"

#include <string>

namespace chess {

//  Standard algebraic notation (SAN), the notation of chess books, websites and PGN files:
//
//      e4  Nf3  exd5  O-O  O-O-O  e8=Q  Qxf7#
//
//  The piece letter (none for pawns), x for a capture, the target square, =Q for a promotion,
//  then + if the move gives check or # if it mates. A pawn capture starts with the pawn's file
//  (exd5). When two pieces of the same type can reach the target, the moving piece's file is
//  added (Nbd2), or its rank if both are on the same file (R1a3), or both if neither is enough
//  (Qa1b2). Only legal moves count, a pinned piece does not make a move ambiguous.
//
//  m must be legal in board. board is not changed: the check and mate suffix is found on a copy.
std::string toSan(const Board& board, Move m);

}  // namespace chess
