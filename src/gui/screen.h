#ifndef SCREEN_H
#define SCREEN_H

#include <SDL2/SDL.h>
#include "point.h"

//  One page of the app (the menu, a game...). App shows one at a time and calls, every frame,
//  handleEvent for each pending event, then update, then draw.

class Screen{
    public:
        virtual ~Screen() = default;

        virtual void handleEvent(const SDL_Event& event) = 0;
        virtual void update(Point mouse) = 0;
        virtual void draw(SDL_Renderer* renderer) = 0;
};

#endif
