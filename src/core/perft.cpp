#include "core/perft.h"

namespace chess {

uint64_t perft(Board& board, int depth) {
    if (depth == 0) return 1;

    MoveList moves;
    board.generateLegalMoves(moves);

    //  Every legal move is one leaf, no need to play them (bulk counting)
    if (depth == 1) return moves.size();

    uint64_t nodes = 0;
    for (Move m : moves) {
        board.makeMove(m);
        nodes += perft(board, depth - 1);
        board.unmakeMove();
    }
    return nodes;
}

uint64_t perftDivide(Board& board, int depth, std::ostream& out) {
    if (depth == 0) return 1;

    MoveList moves;
    board.generateLegalMoves(moves);

    uint64_t total = 0;
    for (Move m : moves) {
        board.makeMove(m);
        uint64_t nodes = perft(board, depth - 1);
        board.unmakeMove();

        out << m.toUci() << ": " << nodes << "\n";
        total += nodes;
    }
    return total;
}

}  // namespace chess
