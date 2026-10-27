#ifndef THEME_H
#define THEME_H

#include "color.h"

//  Every color of the interface, so the look can be changed in one place

namespace theme{

    constexpr Color Background = Color(0x302E2B);

    //  Board
    constexpr Color LightSquare = Color(0xF0D9B5);
    constexpr Color DarkSquare = Color(0xB58863);
    constexpr Color LastMove = Color(0xCDD26A, 150);  // drawn over the from and to squares
    constexpr Color Selected = Color(0x14551E, 110);
    constexpr Color MoveHint = Color(0x14551E, 120);  // dots on empty targets, rings on captures
    constexpr Color BotArrow = Color(0x2F6FD0, 170);  // the move the bot currently prefers
    constexpr Color Check = Color(0xFF0000);  // glow under a king in check

    //  Coordinates use the color of the other square shade so they stay readable on both
    constexpr Color CoordinateOnLight = DarkSquare;
    constexpr Color CoordinateOnDark = LightSquare;

    //  Text and buttons
    constexpr Color Text = Color(0xE8E6E3);
    constexpr Color Button = Color(0x45423E);
    constexpr Color ButtonHover = Color(0x5A5650);
    constexpr Color ButtonText = Text;
    constexpr Color ButtonDisabled = Color(0x3A3835);
    constexpr Color ButtonTextDisabled = Color(0x7A7671);
    constexpr Color ButtonSelected = Color(0x5D8A2F);  // the chosen option of a group
    constexpr Color ButtonSelectedHover = Color(0x6E9E3A);

    //  The bot panel next to the board
    constexpr Color Panel = Color(0x262421);
    constexpr Color PanelLabel = Color(0x9E9A94);

    constexpr Color PickerBackground = Color(0xE8E6E3);
    constexpr Color PickerHover = Color(0xCDD26A);

}

#endif
