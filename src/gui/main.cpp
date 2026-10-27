#include <SDL2/SDL.h>
#include "app.h"


//  SDL needs this exact signature: on Windows it renames main to SDL_main and provides the real entry point

int main(int argc, char* argv[]){
    (void)argc;
    (void)argv;

    App app;
    app.run();

    return 0;
}
