

#include "GameState.h"


GameState::GameState(int castlingRights, int turnToMove, bool enPassantActive, Position enPassantTarget, unsigned int nbMoves, unsigned int nbHalfMoves){
    this->castlingRights = castlingRights;
    this->enPassantActive = enPassantActive;
    this->enPassantTarget = enPassantTarget;

    this->nbMoves = nbMoves;
    this->nbHalfMoves = nbHalfMoves;
}

GameState::GameState(){

}