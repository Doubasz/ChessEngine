#include "board.h"
#include <iostream>

//  Checks getGameStatus on positions with a known result, exits with 1 if any of them is wrong

struct StatusCase{
    const char* name;
    const char* fen;
    GameStatus expected;
};

static const char* statusName(GameStatus s){
    switch(s){
        case ONGOING:          return "ONGOING";
        case CHECKMATE:        return "CHECKMATE";
        case STALEMATE:        return "STALEMATE";
        case DRAW_FIFTY_MOVES: return "DRAW_FIFTY_MOVES";
    }
    return "?";
}

int main(){
    const StatusCase cases[] = {
        {"starting position",           "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", ONGOING},
        {"fool's mate",                 "rnb1kbnr/pppp1ppp/8/4p3/6Pq/5P2/PPPPP2P/RNBQKBNR w KQkq - 1 3", CHECKMATE},
        {"back rank mate on black",     "R5k1/5ppp/8/8/8/8/8/6K1 b - - 0 1", CHECKMATE},
        {"back rank mate on white",     "6k1/8/8/8/8/8/5PPP/r5K1 w - - 0 1", CHECKMATE},
        {"king stalemated in a corner", "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", STALEMATE},
        {"king stalemated by a pawn",   "8/8/8/8/8/1k6/p7/K7 w - - 0 1", STALEMATE},
        {"check with an escape",        "4k3/8/8/8/8/8/8/4R1K1 b - - 0 1", ONGOING},
        {"fifty move rule",             "4k3/8/8/8/8/8/8/4K2R w - - 100 80", DRAW_FIFTY_MOVES},
        {"mate wins over fifty moves",  "R5k1/5ppp/8/8/8/8/8/6K1 b - - 100 80", CHECKMATE},
    };

    int failures = 0;
    for(const StatusCase& c : cases){
        Board board;
        board.parseFENPosition(c.fen);
        GameStatus got = board.getGameStatus();

        if(got != c.expected){
            std::cout << "FAIL " << c.name << ": expected " << statusName(c.expected) << ", got " << statusName(got) << "\n";
            failures++;
        }
        else std::cout << "ok   " << c.name << "\n";
    }

    return failures == 0 ? 0 : 1;
}
