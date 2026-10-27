#ifndef COLOR_H
#define COLOR_H

#include <cstdint>

//  An RGBA color. Colors are written as 0xRRGGBB like in CSS, with an optional alpha
//  (0 is fully transparent, 255 opaque).

struct Color{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;

    constexpr Color() : r(255), g(255), b(255), a(255){}
    constexpr Color(uint32_t rgb, uint8_t alpha = 255)
        : r((rgb >> 16) & 0xFF), g((rgb >> 8) & 0xFF), b(rgb & 0xFF), a(alpha){}
};

#endif
