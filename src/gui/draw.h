#ifndef DRAW_H
#define DRAW_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <string>
#include <vector>
#include "core/board.h"
#include "button.h"
#include "color.h"
#include "point.h"


//  Everything loaded once at startup and freed by freeAssets

struct Assets{
    std::vector<SDL_Texture*> pieces;  // indexed by textureIndex (draw.cpp)

    //  White shapes with smooth edges, tinted when drawn
    SDL_Texture* dot = nullptr;  // legal move to an empty square
    SDL_Texture* ring = nullptr;  // legal capture
    SDL_Texture* glow = nullptr;  // king in check

    TTF_Font* logoFont = nullptr;  // the big title of the menu
    TTF_Font* titleFont = nullptr;
    TTF_Font* infoFont = nullptr;  // the small text of the bot panel
    TTF_Font* buttonFont = nullptr;
    TTF_Font* coordinateFont = nullptr;
};

//  false if a texture or a font could not be loaded, SDL_GetError / TTF_GetError say why
bool loadAssets(SDL_Renderer* renderer, Assets& assets);
void freeAssets(Assets& assets);


//  What to show on top of the plain board
struct BoardHighlights{
    chess::Move lastMove;  // null if none
    chess::Square selected = chess::NoSquare;
    chess::Square checkedKing = chess::NoSquare;
    chess::Square dragged = chess::NoSquare;  // its piece follows the mouse, it is not drawn on the square
    std::vector<chess::Square> quietTargets;  // legal moves of the selected piece to empty squares
    std::vector<chess::Square> captureTargets;
};


//  Screen <-> board coordinates. Rank 8 is drawn at the top, so screen row 0 is rank 8, unless
//  flipped (playing black): then rank 1 is at the top and file h on the left.
//  squareAt returns chess::NoSquare outside the board.
SDL_Rect squareRect(chess::Square s, bool flipped = false);
chess::Square squareAt(Point p, bool flipped = false);

//  The bar above the board where the player picks the promotion piece, in the order of
//  PromotionChoices. promotionChoiceAt returns chess::NoPieceType outside the bar.
extern const chess::PieceType PromotionChoices[4];
SDL_Rect promotionPickerRect();
chess::PieceType promotionChoiceAt(Point p);

void drawBackground(SDL_Renderer* renderer);
void drawBoard(SDL_Renderer* renderer, const chess::Board& board, const BoardHighlights& highlights, bool flipped, const Assets& assets);
void drawArrow(SDL_Renderer* renderer, chess::Square from, chess::Square to, Color color, bool flipped);
void drawPiece(SDL_Renderer* renderer, chess::Piece piece, SDL_Rect rect, const Assets& assets);
void drawButtons(SDL_Renderer* renderer, const std::vector<Button>& buttons, TTF_Font* font);
void drawPromotion(SDL_Renderer* renderer, chess::Color side, Point mouse, const Assets& assets);

void drawRect(SDL_Renderer* renderer, SDL_Rect rect, Color color);

enum class Align { Left, Center, Right };

//  Text vertically centered in area and aligned horizontally, scaled down if it does not fit
void drawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, Color color, SDL_Rect area,
              Align align = Align::Center);

//  Called once per frame after it is shown: frees the cached text that was not drawn in it
void endFrame();

#endif
