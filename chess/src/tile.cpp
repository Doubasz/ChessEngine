
#include "tile.h"

void Tile::setPiece(int p, int c){
    piece = p;
    color = c;
    neverMoved = false;
}

void Tile::setPiece(Tile t){
    piece = t.piece;
    color = t.color;
    neverMoved = false;
    t.neverMoved = false;
}

Tile::Tile(){
    piece = NONE;
    color = -1;
    neverMoved = false;
}

Tile::Tile(int p, int color){setPiece(p, color);}

bool Tile::notBlockedBy(Tile t){
    return (t.piece == NONE || isEnnemy(t));
}

bool Tile::isEnnemy(Tile t){
    return ((color == WHITE && t.color == BLACK) || (color == BLACK && t.color == WHITE));
}


bool Tile::isInLegalMoves(Move p){
    for(Move move : legalMoves){
        if(move == p) return true;
    }
    return false;
}

bool Tile::isInLegalMoves(Position trgt){
    for(Move move : legalMoves){
        if(move.target == trgt){
            return true;
        }
    }
    return false;
}

Move Tile::getInLegalMoves(Position trgt){
    for(Move move : legalMoves){
        if(move.target == trgt){
            return move;
        }
    }
    return {-1, {-1, -1} ,{-1, -1}};
}

void Tile::displayLegalMoves(){
    if(legalMoves.empty()){
        std::cout << "empty" << std::endl;
        return;
    } 

    std::cout << "legalMoves: ";
    for(size_t i = 0; i < legalMoves.size(); i++){
        std::cout << legalMoves[i];
        if(i != legalMoves.size() - 1){
            std::cout << ", ";
        }
    }
    std::cout << "\n";
}