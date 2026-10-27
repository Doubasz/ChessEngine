#ifndef _CHESS
#define _CHESS

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include "core/board.h"
#include "draw.h"
#include "button.h"
#include "def.h"
#include "point.h"



class Chess{
    public:
        SDL_Window* window;
        SDL_Renderer* renderer;
        std::vector<SDL_Texture*> textures;
        std::vector<Button> buttons;
        TTF_Font* font;
        chess::Board board;

        bool quit;

        Chess();
        ~Chess();

        void gameLoop();
        void draw();
        void doInput();

        void handleClick(Point mouse);
        Button* getButton(std::string name);
        void runNewGame();
        void runPerft();
        void getFen();
        void printMoves();

    private:
        //  Everything that depends on the position, recomputed by onPositionChanged after every change
        chess::MoveList legalMoves;
        chess::GameStatus gameStatus = chess::GameStatus::Ongoing;

        chess::Square selectedSquare = chess::NoSquare;  // the piece the player clicked, NoSquare if none
        chess::Square pressedSquare = chess::NoSquare;  // where the mouse button went down

        //  Set while the promotion picker is shown: the pawn move waiting for its piece
        chess::Move pendingPromotion;

        //  Moves taken back with undo, the last one is replayed first by redo
        std::vector<chess::Move> redoStack;

        void onPositionChanged();
        void onSquareClicked(chess::Square s);
        void onPromotionClicked(Point mouse);
        void playMove(chess::Move m);
        void undoMove();
        void redoMove();
        std::vector<chess::Square> highlightedSquares() const;
};



#endif
