#include "draw.h"
#include "def.h"
#include "theme.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>


//  In the order of textureIndex: white pawn to king, then black pawn to king

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
};

const chess::PieceType PromotionChoices[4] = {chess::Queen, chess::Rook, chess::Bishop, chess::Knight};

static int textureIndex(chess::Piece piece){
    return chess::colorOf(piece) * 6 + chess::typeOf(piece) - 1;
}



// ---- Generated shapes ---------------------------------------------------------
//
//  SDL2 cannot draw circles, so they are drawn once, pixel by pixel, into white textures that
//  are then tinted with SDL_SetTextureColorMod when drawn. Each pixel's alpha is how much of it
//  the shape covers, which gives smooth (anti-aliased) edges instead of stair steps.

enum class Shape { Dot, Ring, Glow };

static SDL_Texture* createShapeTexture(SDL_Renderer* renderer, Shape shape){
    const int size = TILESIZE;
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, size, size, 32, SDL_PIXELFORMAT_RGBA32);
    if(!surface) return nullptr;

    const float center = size / 2.0f;
    auto* pixels = static_cast<uint32_t*>(surface->pixels);

    for(int y = 0; y < size; y++){
        for(int x = 0; x < size; x++){
            //  Distance from the pixel center to the square center
            float d = std::hypot(x + 0.5f - center, y + 0.5f - center);
            float alpha = 0.0f;

            switch(shape){
                case Shape::Dot: {
                    float radius = size * 0.16f;
                    alpha = std::clamp(radius - d + 0.5f, 0.0f, 1.0f);
                    break;
                }
                case Shape::Ring: {
                    float outer = size * 0.5f, inner = size * 0.42f;
                    alpha = std::clamp(outer - d + 0.5f, 0.0f, 1.0f) * std::clamp(d - inner + 0.5f, 0.0f, 1.0f);
                    break;
                }
                case Shape::Glow: {
                    //  Solid in the middle, then fading out to nothing near the edge of the square
                    float t = d / (size * 0.5f);
                    alpha = std::clamp((1.1f - t) / (1.1f - 0.3f), 0.0f, 1.0f);
                    break;
                }
            }

            pixels[y * (surface->pitch / 4) + x] = SDL_MapRGBA(surface->format, 255, 255, 255, uint8_t(alpha * 255));
        }
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if(texture) SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

static void drawShape(SDL_Renderer* renderer, SDL_Texture* shape, SDL_Rect rect, Color color){
    SDL_SetTextureColorMod(shape, color.r, color.g, color.b);
    SDL_SetTextureAlphaMod(shape, color.a);
    SDL_RenderCopy(renderer, shape, NULL, &rect);
}



// ---- Assets -------------------------------------------------------------------

bool loadAssets(SDL_Renderer* renderer, Assets& assets){
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");

    for(const std::string& path : TEXTURES_PATHS){
        SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "Loading %s", path.c_str());
        SDL_Texture* texture = IMG_LoadTexture(renderer, path.c_str());
        if(!texture) return false;
        assets.pieces.push_back(texture);
    }

    assets.dot = createShapeTexture(renderer, Shape::Dot);
    assets.ring = createShapeTexture(renderer, Shape::Ring);
    assets.glow = createShapeTexture(renderer, Shape::Glow);
    if(!assets.dot || !assets.ring || !assets.glow) return false;

    assets.titleFont = TTF_OpenFont(FONT_PATH, TITLE_FONT_SIZE);
    assets.buttonFont = TTF_OpenFont(FONT_PATH, BUTTON_FONT_SIZE);
    assets.coordinateFont = TTF_OpenFont(FONT_PATH, COORDINATE_FONT_SIZE);
    if(!assets.titleFont || !assets.buttonFont || !assets.coordinateFont) return false;

    TTF_SetFontStyle(assets.coordinateFont, TTF_STYLE_BOLD);
    return true;
}

void freeAssets(Assets& assets){
    for(SDL_Texture* texture : assets.pieces) SDL_DestroyTexture(texture);
    assets.pieces.clear();

    for(SDL_Texture** texture : {&assets.dot, &assets.ring, &assets.glow}){
        if(*texture) SDL_DestroyTexture(*texture);
        *texture = nullptr;
    }
    for(TTF_Font** font : {&assets.titleFont, &assets.buttonFont, &assets.coordinateFont}){
        if(*font) TTF_CloseFont(*font);
        *font = nullptr;
    }
}



// ---- Coordinates --------------------------------------------------------------

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



// ---- Drawing ------------------------------------------------------------------

void drawRect(SDL_Renderer* renderer, SDL_Rect rect, Color color){
    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &rect);
}

void drawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, Color color, SDL_Rect area){
    if(text.empty()) return;

    //  Blended gives smooth edges, unlike Solid
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), SDL_Color{color.r, color.g, color.b, color.a});
    if(!surface) return;

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);

    //  Keep the text at its real size (no stretching), only shrink it if it is too wide
    float scale = std::min(1.0f, std::min(float(area.w) / surface->w, float(area.h) / surface->h));
    int w = int(surface->w * scale), h = int(surface->h * scale);
    SDL_Rect target = {area.x + (area.w - w) / 2, area.y + (area.h - h) / 2, w, h};

    SDL_FreeSurface(surface);
    if(!texture) return;
    SDL_RenderCopy(renderer, texture, NULL, &target);
    SDL_DestroyTexture(texture);
}

void drawBackground(SDL_Renderer* renderer){
    SDL_SetRenderDrawColor(renderer, theme::Background.r, theme::Background.g, theme::Background.b, 255);
    SDL_RenderClear(renderer);
}

//  Letters a-h along the bottom edge and numbers 1-8 along the left edge, inside the corner
//  of the squares like most chess sites

static void drawCoordinates(SDL_Renderer* renderer, TTF_Font* font){
    const int margin = 3;
    const int box = 16;

    for(int i = 0; i < BOARD_SIZE; i++){
        chess::Square fileSquare = chess::makeSquare(i, 0);  // a1 to h1
        chess::Square rankSquare = chess::makeSquare(0, i);  // a1 to a8

        auto colorOn = [](chess::Square s){
            bool dark = (chess::fileOf(s) + chess::rankOf(s)) % 2 == 0;
            return dark ? theme::CoordinateOnDark : theme::CoordinateOnLight;
        };

        SDL_Rect f = squareRect(fileSquare);
        drawText(renderer, font, std::string(1, char('a' + i)), colorOn(fileSquare),
                 SDL_Rect{f.x + f.w - box - margin, f.y + f.h - box - margin, box, box});

        SDL_Rect r = squareRect(rankSquare);
        drawText(renderer, font, std::string(1, char('1' + i)), colorOn(rankSquare),
                 SDL_Rect{r.x + margin, r.y + margin, box, box});
    }
}

//  Layers from bottom to top: squares, last move and selection tints, check glow, pieces,
//  coordinates (so pieces do not hide them), then the move hints so a capture ring shows
//  around the piece it takes

void drawBoard(SDL_Renderer* renderer, const chess::Board& board, const BoardHighlights& highlights, const Assets& assets){
    for(int rank = 0; rank < BOARD_SIZE; rank++){
        for(int file = 0; file < BOARD_SIZE; file++){
            chess::Square s = chess::makeSquare(file, rank);
            bool dark = (rank + file) % 2 == 0;  // a1 is dark
            drawRect(renderer, squareRect(s), dark ? theme::DarkSquare : theme::LightSquare);
        }
    }

    if(!highlights.lastMove.isNull()){
        drawRect(renderer, squareRect(highlights.lastMove.from()), theme::LastMove);
        drawRect(renderer, squareRect(highlights.lastMove.to()), theme::LastMove);
    }
    if(highlights.selected != chess::NoSquare){
        drawRect(renderer, squareRect(highlights.selected), theme::Selected);
    }
    if(highlights.checkedKing != chess::NoSquare){
        drawShape(renderer, assets.glow, squareRect(highlights.checkedKing), theme::Check);
    }

    for(int rank = 0; rank < BOARD_SIZE; rank++){
        for(int file = 0; file < BOARD_SIZE; file++){
            chess::Square s = chess::makeSquare(file, rank);
            chess::Piece piece = board.pieceAt(s);
            if(piece == chess::NoPiece) continue;

            SDL_Rect rect = squareRect(s);
            SDL_RenderCopy(renderer, assets.pieces[textureIndex(piece)], NULL, &rect);
        }
    }

    drawCoordinates(renderer, assets.coordinateFont);

    for(chess::Square s : highlights.quietTargets) drawShape(renderer, assets.dot, squareRect(s), theme::MoveHint);
    for(chess::Square s : highlights.captureTargets) drawShape(renderer, assets.ring, squareRect(s), theme::MoveHint);
}

void drawButtons(SDL_Renderer* renderer, const std::vector<Button>& buttons, TTF_Font* font){
    for(const Button& button : buttons){
        if(!button.active) continue;

        drawRect(renderer, button.rect, button.hover ? button.colorHover : button.color);
        drawText(renderer, font, button.text, button.colorText, button.rect);
    }
}

void drawPromotion(SDL_Renderer* renderer, chess::Color side, Point mouse, const Assets& assets){
    SDL_Rect rect = promotionPickerRect();
    drawRect(renderer, rect, theme::PickerBackground);

    rect.w = TILESIZE;
    chess::PieceType hovered = promotionChoiceAt(mouse);

    for(chess::PieceType type : PromotionChoices){
        if(type == hovered) drawRect(renderer, rect, theme::PickerHover);
        SDL_RenderCopy(renderer, assets.pieces[textureIndex(chess::makePiece(side, type))], NULL, &rect);
        rect.x += TILESIZE;
    }
}
