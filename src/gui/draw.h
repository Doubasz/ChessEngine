#ifndef DRAW_H
#define DRAW_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "core/board.h"
#include "button.h"
#include "color.h"
#include "point.h"


//  Screen <-> board coordinates. Rank 8 is drawn at the top, so screen row 0 is rank 8.
//  squareAt returns chess::NoSquare outside the board.
SDL_Rect squareRect(chess::Square s);
chess::Square squareAt(Point p);

//  The bar above the board where the player picks the promotion piece, in the order of
//  PromotionChoices. promotionChoiceAt returns chess::NoPieceType outside the bar.
extern const chess::PieceType PromotionChoices[4];
SDL_Rect promotionPickerRect();
chess::PieceType promotionChoiceAt(Point p);

void drawBackground(SDL_Renderer* renderer);
void drawBoard(SDL_Renderer* renderer, const std::vector<chess::Square>& highlighted);
void drawRect(SDL_Renderer* renderer, SDL_Rect rect, Color color, bool border);
void drawPiece(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect& rect);
void drawPieces(SDL_Renderer* renderer, const chess::Board& board, const std::vector<SDL_Texture*>& textures);
void drawButtons(SDL_Renderer* renderer, const std::vector<Button>& buttons, TTF_Font* font);
void drawButton(SDL_Renderer* renderer, const Button& button, TTF_Font* font);
void drawPromotion(SDL_Renderer* renderer, chess::Color side, const std::vector<SDL_Texture*>& textures);
SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path);
std::vector<SDL_Texture*> loadAllTexture(SDL_Renderer* renderer);

#endif
