#ifndef PERFT_H
#define PERFT_H

#include "board.h"
#include <cstdint>
#include <ostream>

//  Counts the leaf nodes of the move tree at the given depth
uint64_t perft(Board& board, int depth);

//  Same as perft but prints the node count under each root move ("e2e4: 20")
uint64_t dividePerft(Board& board, int depth, std::ostream& out);

#endif
