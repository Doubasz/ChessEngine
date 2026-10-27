#ifndef BUTTON_H
#define BUTTON_H

#include <SDL2/SDL.h>
#include <functional>
#include "point.h"
#include <string>
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

        bool isHover() const;
        bool isMouseOver(Point mouse) const;
        void updateHover(Point mouse);
};

Color darkenColor(Color color, int amount);


#endif