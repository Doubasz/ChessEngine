#include "core/board.h"
#include "search/search.h"

#include <cstdio>
#include <iostream>
#include <string>

//  Usage: bestmove [--depth N] [--time MS] [fen]
//  Searches the position (the starting position if no FEN is given) and prints one line per
//  completed depth, then the best move. Without --depth or --time it searches for 5 seconds.

static std::string formatScore(int score){
    char text[32];
    if(chess::isMateScore(score)) std::snprintf(text, sizeof(text), "mate %d", chess::mateInMoves(score));
    else std::snprintf(text, sizeof(text), "%+.2f", score / 100.0);
    return text;
}

int main(int argc, char* argv[]){
    chess::SearchLimits limits;
    std::string fen;

    for(int i = 1; i < argc; i++){
        std::string arg = argv[i];
        try{
            if(arg == "--depth" && i + 1 < argc) limits.depth = std::stoi(argv[++i]);
            else if(arg == "--time" && i + 1 < argc) limits.time = std::chrono::milliseconds(std::stoi(argv[++i]));
            else{
                //  The FEN contains spaces, accept it either quoted as one argument or as several arguments
                if(!fen.empty()) fen += ' ';
                fen += arg;
            }
        } catch(const std::exception&){
            std::cerr << "Usage: " << argv[0] << " [--depth N] [--time MS] [fen]\n";
            return 1;
        }
    }
    if(limits.depth == chess::MaxPly && limits.time.count() == 0) limits.time = std::chrono::seconds(5);
    if(fen.empty()) fen = std::string(chess::Board::StartFen);

    chess::Board board;
    if(!board.setFen(fen)){
        std::cerr << "Invalid FEN: " << fen << "\n";
        return 1;
    }

    chess::Searcher searcher;
    chess::SearchResult result = searcher.search(board, limits, [](const chess::SearchResult& r){
        uint64_t nps = r.elapsed.count() > 0 ? r.nodes * 1000 / r.elapsed.count() : 0;
        std::cout << "depth " << r.depth
                  << "  score " << formatScore(r.score)
                  << "  nodes " << r.nodes
                  << "  time " << r.elapsed.count() << " ms"
                  << "  nps " << nps
                  << "  best " << r.bestMove.toUci() << std::endl;
    });

    std::cout << "bestmove " << result.bestMove.toUci() << "\n";
    return 0;
}
