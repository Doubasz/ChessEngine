#pragma once

#include "core/board.h"

#include <cstdint>
#include <ostream>

namespace chess {

//  Counts the leaf positions of the legal move tree, depth plies deep. The counts for
//  standard positions are well known, so perft is the reference test of a move generator.
uint64_t perft(Board& board, int depth);

//  Same total, also writes the count below each root move ("e2e4: 20") like Stockfish's
//  "go perft", so the output can be diffed against it move by move (tools/perftDiff.py)
uint64_t perftDivide(Board& board, int depth, std::ostream& out);

}  // namespace chess
