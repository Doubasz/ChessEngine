#include "draw.h"
#include "def.h"
#include "board.h"
#include "color.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <vector>
#include <cstring>
#include <iostream>






SDL_Texture* loadTexture(SDL_Renderer* renderer, char* path){
    SDL_Texture* texture;

    SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s", path);

    texture = IMG_LoadTexture(renderer, path);

    return texture;
}



std::vector<SDL_Texture*> loadAllTexture(SDL_Renderer* renderer){
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    std::vector<SDL_Texture*> textures;

    for(int i = 0; i < NUM_PATHS; i++){

        const char* temp = TEXTURES_PATHS[i].c_str();

        char* path = new char[std::strlen(temp) + 1];
        std::strcpy(path, temp);

        textures.push_back(loadTexture(renderer, path));

        delete[] path;
    }
    std::cout << "size vector: " << textures.size() << std::endl; 
    return textures;
}



void drawBackground(SDL_Renderer*& renderer){

    Color bg = Color(BG_COLOR);

    SDL_SetRenderDrawColor(renderer, bg.b, bg.g, bg.r, 255);
    SDL_RenderClear(renderer);

}



void drawBoard(SDL_Renderer*& renderer, Board& board, Tile* selectedPiece){

    Color black = Color(BLACK_TILE);
    Color white = Color(WHITE_TILE);
    Color red = Color(RED_MOVE);
    //Color green = Color(0x278c42);
    //Color yellow = Color(0xad953b);

    Tile temp;
    if (selectedPiece != nullptr) temp = *selectedPiece;


    SDL_Rect tile = SDL_Rect{STARTING_POSX, STARTING_POSY, TILESIZE, TILESIZE};

    for (int i = 0; i < BOARD; i++){
        
        tile.x = STARTING_POSX;

        for(int j = 0; j < BOARD; j++){
            Color currentColor = Color();
            currentColor = (((i + j) % 2) == 0) ? white : black;
            
            if(!temp.legalMoves.empty()){
                if(temp.isInLegalMoves({i, j})){
                    currentColor = red;
                }
            }

            drawRect(renderer, tile, currentColor, true);
            
            tile.x += 75;
        }
        tile.y += 75;
    }
}



void drawPieces(SDL_Renderer*& renderer, Board& board, std::vector<SDL_Texture*> textures){
    for(int i = 0; i < BOARD; i++){
        for(int j = 0; j < BOARD; j++){
            SDL_Rect tile = SDL_Rect{board.posTiles[i][j].x, board.posTiles[i][j].y, TILESIZE, TILESIZE};

            if (board.tiles[i][j].piece != 0) {
                drawPiece(renderer, textures[(board.tiles[i][j].piece + board.tiles[i][j].color) - 1], tile);
            }
        }
    }

}

void drawButtons(SDL_Renderer*& renderer, std::vector<Button> buttons, TTF_Font* font){
    for(Button button : buttons){
        if(button.active){
            drawButton(renderer, button, font);
        }
    }
}



void drawButton(SDL_Renderer*& renderer, Button button, TTF_Font* font){

    if(button.isHover()){
        drawRect(renderer, button.rect, button.colorHover, button.border);
    }
    else drawRect(renderer, button.rect, button.color, button.border);

    if(button.text != ""){
        SDL_Surface* textSurface = TTF_RenderText_Solid(font, button.text.c_str(), SDL_Color{button.colorText.r, button.colorText.g, button.colorText.b});

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


void drawRect(SDL_Renderer*& renderer, SDL_Rect rect, Color color, bool border){

    SDL_SetRenderDrawColor(renderer, color.b, color.g, color.r, 255);
    SDL_RenderFillRect(renderer, &rect);

    if(border){
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawRect(renderer, &rect);
    }
    
    
}



void drawPiece(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect& rect){

    int textureWidth, textureHeight;
    SDL_QueryTexture(texture, NULL, NULL, &textureWidth, &textureHeight);
    SDL_RenderCopy(renderer, texture, NULL, &rect);
}




void drawPromotion(SDL_Renderer*& renderer, Board& board, Tile target, std::vector<SDL_Texture*> textures){
    int x = (STARTING_POSX + (TILESIZE * BOARD)) / 2; // get the middle of the board (used it as a default value)
    int y = STARTING_POSY - (20 + TILESIZE); // up the y of the board by 20, why 20? because it looks good

    int width = (TILESIZE * 4); // 4 is the number of piece (q, b, n, r)
    int height = TILESIZE;

    Color colorRect = 0xD9D9D9;
    SDL_Rect rect = SDL_Rect{x, y, width, height};

    int colorPiece = target.color;

    drawRect(renderer, rect, colorRect, true);

    rect.w = TILESIZE;

    for(int i = 4; i > 0; i--){
        drawPiece(renderer, textures[i + colorPiece], rect);
        rect.x += TILESIZE;
    }

}