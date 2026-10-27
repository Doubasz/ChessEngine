#ifndef _CHESS
#define _CHESS

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "draw.h"
#include "board.h"
#include "button.h"
#include "def.h"




class Chess{
    public:
        SDL_Window* window;
        SDL_Renderer* renderer;
        std::vector<SDL_Texture*> textures;
        std::vector<Button> buttons;
        TTF_Font* font;
        Board board;
        GameStatus gameStatus;

        bool quit;

        Chess();
        ~Chess();

        void gameLoop();
        void draw();
        void doInput();

        Position getPiecePosInArray(Position posInBoard);
        bool isButtonClicked(Position mouse);
        void handleClick(Position mouse);
        Button* getButton(std::string name);
        void runNewGame();
        void updateGameStatus();
        void runPerft();
        void getFen();
        void printMoves();
};   



bool isInBoard(Position selectPos);
bool isPromotion(Tile target);
bool isBetweenZone(Position posToVerify, SDL_Rect zone);



#endif