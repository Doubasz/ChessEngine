#include "board.h"
#include "perft.h"
#include <iostream>
#include <string>

//  Usage: perft <depth> [fen]
//  Prints the node count under each root move then the total, in the same format as Stockfish's "go perft"
//  so the output can be diffed against it (see tools/perftDiff.py)

static const std::string STARTING_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

int main(int argc, char* argv[]){
    if(argc < 2){
        std::cerr << "Usage: " << argv[0] << " <depth> [fen]\n";
        return 1;
    }

    int depth;
    try{
        depth = std::stoi(argv[1]);
    } catch(const std::exception&){
        std::cerr << "Invalid depth: " << argv[1] << "\n";
        return 1;
    }

    //  The FEN contains spaces, accept it either quoted as one argument or as several arguments
    std::string fen;
    for(int i = 2; i < argc; i++){
        if(!fen.empty()) fen += ' ';
        fen += argv[i];
    }
    if(fen.empty()) fen = STARTING_FEN;

    Board board;
    board.parseFENPosition(fen);

    uint64_t nodes = dividePerft(board, depth, std::cout);
    std::cout << "\nNodes searched: " << nodes << "\n";

    return 0;
}
