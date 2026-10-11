#ifndef GAMESTATE_H
#define GAMESTATE_H

#include "position.h"

class GameState{

    public:
        int castlingRights;
        bool enPassantActive;
        Position enPassantTarget;

        unsigned int nbMoves;
        unsigned int nbHalfMoves;


        GameState(int castlingRights, int turnToMove, bool enPassantActive, Position enPassantTarget, unsigned int nbMoves, unsigned int nbHalfMoves);
        GameState();

};




#endif

