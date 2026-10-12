#ifndef COLOR_H
#define COLOR_H


#include <cstdint>

class Color{
    public:
        uint8_t r;
        uint8_t g;
        uint8_t b;
        uint32_t hex;

        Color();
        Color(uint8_t r, uint8_t g, uint8_t b);
        Color(uint32_t hexa);
};

int power(int a, int n);

#endif