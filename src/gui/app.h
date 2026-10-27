#ifndef APP_H
#define APP_H

#include <SDL2/SDL.h>
#include <memory>
#include "draw.h"
#include "screen.h"

//  Owns the window, the renderer and the assets, and runs the current screen.
//  Screens ask for a switch with showMenu / startGame, it happens at the end of the frame
//  so a screen is never destroyed while one of its own buttons is running.

class App{
    public:
        App();
        ~App();

        App(const App&) = delete;
        App& operator=(const App&) = delete;

        void run();

        void showMenu();
        void startGame();
        void quit();

        const Assets& assets() const;

    private:
        SDL_Window* window = nullptr;
        SDL_Renderer* renderer = nullptr;
        Assets loadedAssets;

        std::unique_ptr<Screen> screen;
        std::unique_ptr<Screen> nextScreen;  // replaces screen at the end of the frame
        bool running = true;
};

#endif
