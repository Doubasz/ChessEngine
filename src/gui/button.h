#ifndef BUTTON_H
#define BUTTON_H

#include <SDL2/SDL.h>
#include <functional>
#include "position.h"
#include "color.h"

class Button{
    public:
        SDL_Rect rect;
        std::string name;
        std::string text;
        bool hover;
        bool active;
        bool border;
        Color color;
        Color colorHover;
        Color colorText;
        std::function<void()> onClick;

    
        Button();
        Button(int x, int y, int w, int h);
        ~Button();

        bool isHover();
        bool isMouseOver(Position mouse);
        void updateHover(Position mouse);
};

Color darkenColor(Color color, int amount);


#endif