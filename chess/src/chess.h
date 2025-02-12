#ifndef _CHESS
#define _CHESS

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "draw.h"
#include "board.h"
#include "button.h"




class Chess{
    public:
        SDL_Window* window;
        SDL_Renderer* renderer;
        std::vector<SDL_Texture*> textures;
        std::vector<Button> buttons;
        TTF_Font* font;
        Board board;

        bool quit;

        Chess();
        ~Chess();

        void gameLoop();
        void draw();
        void doInput();

        Position getPiecePosInArray(Position posInBoard);
        Position getClosestTile(Position p);
        bool isButtonClicked(Position mouse);
        void handleClick(Position mouse);
        Button* getButton(std::string name);
        void runNewGame();
        uint64_t perft(int depth);
        uint64_t dividePerft(int depth);
        void runPerft();
};   



bool isInBoard(Position selectPos);
bool isInTile(Position selectPos, Position tilePos);
bool isPromotion(Tile target);
bool isBetweenZone(Position posToVerify, SDL_Rect zone);



#endif