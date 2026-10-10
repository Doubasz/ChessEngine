#include "perft.h"

uint64_t perft(Board& board, int depth){
    if(depth == 0) return 1;

    Move moves[256];
    uint64_t totalNodes = 0;

    int nbMoves = board.getAllMoves(moves, 256);

    for(int i = 0; i < nbMoves; i++){
        board.makeMove(moves[i]);
        totalNodes += perft(board, depth - 1);
        board.undoMove(moves[i]);
    }

    return totalNodes;
}

uint64_t dividePerft(Board& board, int depth, std::ostream& out){
    if(depth == 0) return 1;

    Move moves[256];
    uint64_t totalNodes = 0;

    int nbMoves = board.getAllMoves(moves, 256);

    for(int i = 0; i < nbMoves; i++){
        board.makeMove(moves[i]);
        uint64_t nodes = perft(board, depth - 1);
        board.undoMove(moves[i]);

        out << moves[i].moveToAlbr() << ": " << nodes << "\n";
        totalNodes += nodes;
    }

    out << "Total: " << totalNodes << "\n";
    return totalNodes;
}
