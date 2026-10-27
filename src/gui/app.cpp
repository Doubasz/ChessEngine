#include "app.h"
#include "def.h"
#include "game_screen.h"
#include "menu_screen.h"
#include <SDL2/SDL_ttf.h>
#include <iostream>



App::App(){
    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        exit(1);
    }

    if(TTF_Init() == -1){
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        exit(1);
    }

    window = SDL_CreateWindow("Chess", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, 0);
    //  VSync shows each frame in step with the screen refresh: no tearing and an even pace
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if(!window || !renderer){
        std::cerr << "Failed to create the window: " << SDL_GetError() << std::endl;
        exit(1);
    }

    //  Blending so the colors with an alpha below 255 are see-through
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if(!loadAssets(renderer, loadedAssets)){
        std::cerr << "Failed to load assets: " << SDL_GetError() << " " << TTF_GetError() << std::endl;
        exit(1);
    }

    showMenu();
}

App::~App(){
    //  The screens may hold textures, they go before the renderer
    screen.reset();
    nextScreen.reset();

    freeAssets(loadedAssets);
    if(renderer) SDL_DestroyRenderer(renderer);
    if(window) SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
}



void App::showMenu(){
    nextScreen = std::make_unique<MenuScreen>(*this);
}

void App::startGame(const GameSetup& setup){
    nextScreen = std::make_unique<GameScreen>(*this, setup);
}

void App::quit(){
    running = false;
}

const Assets& App::assets() const{
    return loadedAssets;
}



void App::run(){
    const int FPS = 60;
    const Uint32 frameDelay = 1000 / FPS;

    while(running){
        Uint32 frameStart = SDL_GetTicks();

        if(nextScreen) screen = std::move(nextScreen);

        SDL_Event event;
        while(SDL_PollEvent(&event)){
            if(event.type == SDL_QUIT) quit();
            else screen->handleEvent(event);
        }

        Point mouse;
        SDL_GetMouseState(&mouse.x, &mouse.y);
        screen->update(mouse);

        drawBackground(renderer);
        screen->draw(renderer);
        SDL_RenderPresent(renderer);
        endFrame();

        //  Cap FPS, in case VSync is not available (it already waits for the screen otherwise)
        Uint32 frameTime = SDL_GetTicks() - frameStart;
        if(frameTime < frameDelay) SDL_Delay(frameDelay - frameTime);
    }
}
