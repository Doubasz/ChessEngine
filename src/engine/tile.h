#ifndef _TILE
#define _TILE

#define WHITE 0
#define BLACK 6

#define BOARD 8

#include "move.h"
#include <vector>
#include <string>
#include "position.h"


enum Piece{
    NONE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6,
};





class Tile{

    public:

        unsigned int piece;
        int color;
        
        //  Current position in the board [i][j]
        Position currentPos;
        std::vector<Move> legalMoves;
        bool neverMoved;

    public:
        Tile();
        Tile(int p, int color);
        void setPiece(int p, int color);
        void setPiece(Tile p);
        void addMove(Position p);
        bool isInLegalMoves(Move p);
        bool isInLegalMoves(Position trgt);
        bool notBlockedBy(Tile t);
        bool isEnnemy(Tile t);
        Move getInLegalMoves(Position trgt);
        void displayLegalMoves();
};


#endif