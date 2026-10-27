
#include "button.h"
#include "theme.h"

Button::Button() : Button(0, 0, 0, 0){}

Button::Button(int x, int y, int w, int h){
    rect = SDL_Rect{x, y, w, h};
    text = "";
    active = true;
    hover = false;
    border = false;
    color = theme::Button;
    colorHover = theme::ButtonHover;
    colorText = theme::ButtonText;
}



bool Button::isHover() const{
    return hover;
}

bool Button::isMouseOver(Point mouse) const{
    return mouse.x > rect.x && mouse.x < (rect.x + rect.w) && mouse.y > rect.y && mouse.y < (rect.y + rect.h);
}

void Button::updateHover(Point mouse){
    hover = isMouseOver(mouse);
}
