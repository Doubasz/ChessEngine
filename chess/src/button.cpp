

#include "button.h"

Button::Button(){
    rect = SDL_Rect{0, 0, 0, 0};
    text = "";
    active = true;
    hover = false;
    border = true;
    color = Color(0xACACAC);

    int amount = 30;
    colorHover = darkenColor(color, amount);
    colorText = Color(0);
}

Button::Button(int x, int y, int w, int h){
    rect = SDL_Rect{x, y, w, h};
    text = "";
    active = true;
    hover = false;
    border = true;
    color = Color(0xACACAC);

    int amount = 30;
    colorHover = darkenColor(color, amount);
    colorText = Color(0);
}

Button::~Button(){

}



bool Button::isHover(){
    return hover;
}

bool Button::isMouseOver(Position mouse){
    return mouse.x > rect.x && mouse.x < (rect.x + rect.w) && mouse.y > rect.y && mouse.y < (rect.y + rect.h);
}

void Button::updateHover(Position mouse){
    hover = (isMouseOver(mouse)) ? true : false;
}


Color darkenColor(Color color, int amount) {
    return {
        (Uint8)std::max(0, color.r - amount),
        (Uint8)std::max(0, color.g - amount),
        (Uint8)std::max(0, color.b - amount)
    };
}