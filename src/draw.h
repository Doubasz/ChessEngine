#ifndef DRAW_H
#define DRAW_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <vector>
#include "board.h"
#include "button.h"
#include "color.h"



void drawBackground(SDL_Renderer*& renderer);
void drawBoard(SDL_Renderer*& renderer, Board& board, Tile* selectedPiece);
void drawRect(SDL_Renderer*& renderer, SDL_Rect rect, Color color, bool border);
void drawPiece(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect& rect);
void drawPieces(SDL_Renderer*& renderer, Board& board, std::vector<SDL_Texture*> textures);
void drawButtons(SDL_Renderer*& renderer, std::vector<Button> buttons, TTF_Font* font);
void drawButton(SDL_Renderer*& renderer, Button button, TTF_Font* font);
void drawPromotion(SDL_Renderer*& renderer, Board& board, Tile target, std::vector<SDL_Texture*> textures);
SDL_Texture* loadTexture(SDL_Renderer* renderer, char* path);
std::vector<SDL_Texture*> loadAllTexture(SDL_Renderer* renderer);

#endif