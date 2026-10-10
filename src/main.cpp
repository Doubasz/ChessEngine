#include <iostream>
#include <SDL2/SDL.h>
#include "chess.h"


int main(void){

    Chess chess;

    chess.gameLoop();
    //chess.runPerft();

    return 0;
}