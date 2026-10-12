

#include "color.h"


int power(int a, int n){
    int temp = a;
    for(int i = 1; i < n; i++){
        a *= temp;
    }
    return a;
}

Color::Color(){
    hex = 0xFFFFFF;
    r = g = b = 255;
}

Color::Color(uint32_t hex) : hex(hex){
    b = (hex >> 16) & 0xFF;
    g = (hex >> 8) & 0xFF;
    r = hex & 0xFF;
}

Color::Color(uint8_t r, uint8_t g, uint8_t b) : r(r), g(g), b(b){
    hex = r * power(2, 16) + g * power(2, 8) + b;
}