#include <SDL2/SDL.h>
#include "chess.h"


//  SDL needs this exact signature: on Windows it renames main to SDL_main and provides the real entry point

int main(int argc, char* argv[]){
    (void)argc;
    (void)argv;

    Chess chess;

    chess.gameLoop();

    return 0;
}
