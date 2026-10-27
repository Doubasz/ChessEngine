#ifndef BUTTON_H
#define BUTTON_H

#include <SDL2/SDL.h>
#include <functional>
#include <string>
#include <vector>
#include "color.h"
#include "point.h"

class Button{
    public:
        SDL_Rect rect;
        std::string name;
        std::string text;
        bool hover;
        bool active;  // drawn and clickable, false hides it
        bool enabled;  // false shows it greyed out, without hover or click
        bool border;
        Color color;
        Color colorHover;
        Color colorText;
        std::function<void()> onClick;


        Button();
        Button(int x, int y, int w, int h);

        bool isHover() const;
        bool isMouseOver(Point mouse) const;
        void updateHover(Point mouse);
};

//  Runs onClick of the active, enabled button under the mouse. false if there is none.
bool clickButtons(const std::vector<Button>& buttons, Point mouse);

#endif
