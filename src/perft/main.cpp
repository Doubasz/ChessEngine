#include "core/board.h"
#include "core/perft.h"

#include <chrono>
#include <iostream>
#include <string>

//  Usage: perft <depth> [fen]
//  Prints the node count under each root move then the total, in the same format as Stockfish's "go perft"
//  so the output can be diffed against it (see tools/perftDiff.py). The timing goes to stderr so stdout
//  stays in that format.

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
    if(fen.empty()) fen = std::string(chess::Board::StartFen);

    chess::Board board;
    if(!board.setFen(fen)){
        std::cerr << "Invalid FEN: " << fen << "\n";
        return 1;
    }

    auto start = std::chrono::steady_clock::now();
    uint64_t nodes = chess::perftDivide(board, depth, std::cout);
    double seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    std::cout << "\nNodes searched: " << nodes << "\n";
    std::cerr << "Time: " << seconds << " s (" << uint64_t(nodes / (seconds > 0 ? seconds : 1e-9) / 1000) << " thousand nodes per second)\n";

    return 0;
}
