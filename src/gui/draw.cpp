#include "draw.h"
#include "def.h"
#include "color.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <vector>
#include <iostream>
#include <string>


//  Pieces first, in the order of textureIndex: white pawn to king, then black pawn to king

static const std::string TEXTURES_PATHS[NUM_PATHS] = {
    WPAWN_PATH,
    WKNIGHT_PATH,
    WBISHOP_PATH,
    WROOK_PATH,
    WQUEEN_PATH,
    WKING_PATH,
    BPAWN_PATH,
    BKNIGHT_PATH,
    BBISHOP_PATH,
    BROOK_PATH,
    BQUEEN_PATH,
    BKING_PATH,
    UNDO_REDO,
};

const chess::PieceType PromotionChoices[4] = {chess::Queen, chess::Rook, chess::Bishop, chess::Knight};

static int textureIndex(chess::Piece piece){
    return chess::colorOf(piece) * 6 + chess::typeOf(piece) - 1;
}



SDL_Rect squareRect(chess::Square s){
    int row = 7 - chess::rankOf(s);
    return SDL_Rect{STARTING_POSX + chess::fileOf(s) * TILESIZE, STARTING_POSY + row * TILESIZE, TILESIZE, TILESIZE};
}

chess::Square squareAt(Point p){
    bool inside = p.x >= STARTING_POSX && p.y >= STARTING_POSY
               && p.x < STARTING_POSX + TILESIZE * BOARD_SIZE && p.y < STARTING_POSY + TILESIZE * BOARD_SIZE;
    if(!inside) return chess::NoSquare;

    int file = (p.x - STARTING_POSX) / TILESIZE;
    int row = (p.y - STARTING_POSY) / TILESIZE;
    return chess::makeSquare(file, 7 - row);
}

SDL_Rect promotionPickerRect(){
    int x = (STARTING_POSX + (TILESIZE * BOARD_SIZE)) / 2;
    int y = STARTING_POSY - (20 + TILESIZE);  // 20 pixels above the board
    return SDL_Rect{x, y, TILESIZE * 4, TILESIZE};
}

chess::PieceType promotionChoiceAt(Point p){
    SDL_Rect picker = promotionPickerRect();
    if(p.x < picker.x || p.x >= picker.x + picker.w || p.y < picker.y || p.y >= picker.y + picker.h){
        return chess::NoPieceType;
    }
    return PromotionChoices[(p.x - picker.x) / TILESIZE];
}



SDL_Texture* loadTexture(SDL_Renderer* renderer, const char* path){
    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s", path);
    return IMG_LoadTexture(renderer, path);
}

std::vector<SDL_Texture*> loadAllTexture(SDL_Renderer* renderer){
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    std::vector<SDL_Texture*> textures;
    for(const std::string& path : TEXTURES_PATHS){
        textures.push_back(loadTexture(renderer, path.c_str()));
    }
    return textures;
}



void drawBackground(SDL_Renderer* renderer){
    Color bg = Color(BG_COLOR);

    SDL_SetRenderDrawColor(renderer, bg.b, bg.g, bg.r, 255);
    SDL_RenderClear(renderer);
}

//  The squares, the highlighted ones (legal targets of the selected piece) in red

void drawBoard(SDL_Renderer* renderer, const std::vector<chess::Square>& highlighted){
    Color dark = Color(BLACK_TILE);
    Color light = Color(WHITE_TILE);
    Color red = Color(RED_MOVE);

    for(int rank = 0; rank < BOARD_SIZE; rank++){
        for(int file = 0; file < BOARD_SIZE; file++){
            chess::Square s = chess::makeSquare(file, rank);

            //  a1 is a dark square
            Color color = ((rank + file) % 2 == 0) ? dark : light;
            if(std::find(highlighted.begin(), highlighted.end(), s) != highlighted.end()) color = red;

            drawRect(renderer, squareRect(s), color, true);
        }
    }
}

void drawPieces(SDL_Renderer* renderer, const chess::Board& board, const std::vector<SDL_Texture*>& textures){
    for(int rank = 0; rank < BOARD_SIZE; rank++){
        for(int file = 0; file < BOARD_SIZE; file++){
            chess::Square s = chess::makeSquare(file, rank);
            chess::Piece piece = board.pieceAt(s);

            if(piece != chess::NoPiece){
                SDL_Rect rect = squareRect(s);
                drawPiece(renderer, textures[textureIndex(piece)], rect);
            }
        }
    }
}

void drawButtons(SDL_Renderer* renderer, const std::vector<Button>& buttons, TTF_Font* font){
    for(const Button& button : buttons){
        if(button.active){
            drawButton(renderer, button, font);
        }
    }
}

void drawButton(SDL_Renderer* renderer, const Button& button, TTF_Font* font){

    drawRect(renderer, button.rect, button.hover ? button.colorHover : button.color, button.border);

    if(button.text != ""){
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, button.text.c_str(), SDL_Color{button.colorText.r, button.colorText.g, button.colorText.b, 255});

        if (!textSurface) {
            std::cerr << "Failed to create text surface: " << TTF_GetError() << std::endl;
            exit(1);
        }

        SDL_Texture* textTexture = SDL_CreateTextureFromSurface(renderer, textSurface);
        SDL_FreeSurface(textSurface);

        SDL_RenderCopy(renderer, textTexture, NULL, &button.rect);
        SDL_DestroyTexture(textTexture);
    }
}

void drawRect(SDL_Renderer* renderer, SDL_Rect rect, Color color, bool border){

    SDL_SetRenderDrawColor(renderer, color.b, color.g, color.r, 255);
    SDL_RenderFillRect(renderer, &rect);

    if(border){
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
}

void drawPiece(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect& rect){
    SDL_RenderCopy(renderer, texture, NULL, &rect);
}

void drawPromotion(SDL_Renderer* renderer, chess::Color side, const std::vector<SDL_Texture*>& textures){
    SDL_Rect rect = promotionPickerRect();
    drawRect(renderer, rect, Color(0xD9D9D9), true);

    rect.w = TILESIZE;
    for(chess::PieceType type : PromotionChoices){
        drawPiece(renderer, textures[textureIndex(chess::makePiece(side, type))], rect);
        rect.x += TILESIZE;
    }
}
