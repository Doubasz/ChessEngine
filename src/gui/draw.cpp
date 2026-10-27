#include "draw.h"
#include "def.h"
#include "theme.h"
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>
#include <unordered_map>
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



//  Text textures kept between frames, see drawText

namespace {

struct CachedText{
    SDL_Texture* texture = nullptr;
    int w = 0;
    int h = 0;
    bool usedThisFrame = false;
};

std::unordered_map<std::string, CachedText> textCache;

std::string textKey(TTF_Font* font, const std::string& text, Color color){
    char prefix[64];
    std::snprintf(prefix, sizeof(prefix), "%p/%02x%02x%02x%02x/", static_cast<void*>(font), color.r, color.g, color.b, color.a);
    return prefix + text;
}

}  // namespace



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

    assets.logoFont = TTF_OpenFont(FONT_PATH, LOGO_FONT_SIZE);
    assets.infoFont = TTF_OpenFont(FONT_PATH, INFO_FONT_SIZE);
    assets.titleFont = TTF_OpenFont(FONT_PATH, TITLE_FONT_SIZE);
    assets.buttonFont = TTF_OpenFont(FONT_PATH, BUTTON_FONT_SIZE);
    assets.coordinateFont = TTF_OpenFont(FONT_PATH, COORDINATE_FONT_SIZE);
    if(!assets.logoFont || !assets.infoFont || !assets.titleFont || !assets.buttonFont || !assets.coordinateFont) return false;

    TTF_SetFontStyle(assets.logoFont, TTF_STYLE_BOLD);
    TTF_SetFontStyle(assets.coordinateFont, TTF_STYLE_BOLD);
    return true;
}

void freeAssets(Assets& assets){
    for(auto& entry : textCache) SDL_DestroyTexture(entry.second.texture);
    textCache.clear();

    for(SDL_Texture* texture : assets.pieces) SDL_DestroyTexture(texture);
    assets.pieces.clear();

    for(SDL_Texture** texture : {&assets.dot, &assets.ring, &assets.glow}){
        if(*texture) SDL_DestroyTexture(*texture);
        *texture = nullptr;
    }
    for(TTF_Font** font : {&assets.logoFont, &assets.infoFont, &assets.titleFont, &assets.buttonFont, &assets.coordinateFont}){
        if(*font) TTF_CloseFont(*font);
        *font = nullptr;
    }
}



// ---- Coordinates --------------------------------------------------------------

SDL_Rect squareRect(chess::Square s, bool flipped){
    int column = flipped ? 7 - chess::fileOf(s) : chess::fileOf(s);
    int row = flipped ? chess::rankOf(s) : 7 - chess::rankOf(s);
    return SDL_Rect{STARTING_POSX + column * TILESIZE, STARTING_POSY + row * TILESIZE, TILESIZE, TILESIZE};
}

chess::Square squareAt(Point p, bool flipped){
    bool inside = p.x >= STARTING_POSX && p.y >= STARTING_POSY
               && p.x < STARTING_POSX + TILESIZE * BOARD_SIZE && p.y < STARTING_POSY + TILESIZE * BOARD_SIZE;
    if(!inside) return chess::NoSquare;

    int column = (p.x - STARTING_POSX) / TILESIZE;
    int row = (p.y - STARTING_POSY) / TILESIZE;
    return flipped ? chess::makeSquare(7 - column, row) : chess::makeSquare(column, 7 - row);
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

//  Rendering text with SDL_ttf is slow (about 0.3 ms each, and a frame shows dozens), so each
//  text is rendered once into a texture and reused for as long as it stays on screen.
//  endFrame() destroys the ones that were not drawn during the frame.

void drawText(SDL_Renderer* renderer, TTF_Font* font, const std::string& text, Color color, SDL_Rect area, Align align){
    if(text.empty()) return;

    CachedText& cached = textCache[textKey(font, text, color)];
    if(!cached.texture){
        //  Blended gives smooth edges, unlike Solid
        SDL_Surface* surface = TTF_RenderUTF8_Blended(font, text.c_str(), SDL_Color{color.r, color.g, color.b, color.a});
        if(!surface) return;
        cached.texture = SDL_CreateTextureFromSurface(renderer, surface);
        cached.w = surface->w;
        cached.h = surface->h;
        SDL_FreeSurface(surface);
        if(!cached.texture) return;
    }
    cached.usedThisFrame = true;

    //  Keep the text at its real size (no stretching), only shrink it if it is too wide
    float scale = std::min(1.0f, std::min(float(area.w) / cached.w, float(area.h) / cached.h));
    int w = int(cached.w * scale), h = int(cached.h * scale);
    int x = align == Align::Left ? area.x : align == Align::Right ? area.x + area.w - w : area.x + (area.w - w) / 2;
    SDL_Rect target = {x, area.y + (area.h - h) / 2, w, h};

    SDL_RenderCopy(renderer, cached.texture, NULL, &target);
}

void endFrame(){
    for(auto it = textCache.begin(); it != textCache.end();){
        if(it->second.usedThisFrame){
            it->second.usedThisFrame = false;
            ++it;
        }
        else{
            SDL_DestroyTexture(it->second.texture);
            it = textCache.erase(it);
        }
    }
}

void drawBackground(SDL_Renderer* renderer){
    SDL_SetRenderDrawColor(renderer, theme::Background.r, theme::Background.g, theme::Background.b, 255);
    SDL_RenderClear(renderer);
}

//  Letters along the bottom edge and numbers along the left edge, inside the corner of the
//  squares like most chess sites. Flipped, the bottom row is rank 8 and the left column file h.

static void drawCoordinates(SDL_Renderer* renderer, TTF_Font* font, bool flipped){
    const int margin = 3;
    const int box = 16;
    const int edge = flipped ? 7 : 0;

    for(int i = 0; i < BOARD_SIZE; i++){
        chess::Square fileSquare = chess::makeSquare(i, edge);  // along the bottom row
        chess::Square rankSquare = chess::makeSquare(edge, i);  // along the left column

        auto colorOn = [](chess::Square s){
            bool dark = (chess::fileOf(s) + chess::rankOf(s)) % 2 == 0;
            return dark ? theme::CoordinateOnDark : theme::CoordinateOnLight;
        };

        SDL_Rect f = squareRect(fileSquare, flipped);
        drawText(renderer, font, std::string(1, char('a' + i)), colorOn(fileSquare),
                 SDL_Rect{f.x + f.w - box - margin, f.y + f.h - box - margin, box, box});

        SDL_Rect r = squareRect(rankSquare, flipped);
        drawText(renderer, font, std::string(1, char('1' + i)), colorOn(rankSquare),
                 SDL_Rect{r.x + margin, r.y + margin, box, box});
    }
}

//  Layers from bottom to top: squares, last move and selection tints, check glow, pieces,
//  coordinates (so pieces do not hide them), then the move hints so a capture ring shows
//  around the piece it takes

void drawBoard(SDL_Renderer* renderer, const chess::Board& board, const BoardHighlights& highlights, bool flipped, const Assets& assets){
    for(int rank = 0; rank < BOARD_SIZE; rank++){
        for(int file = 0; file < BOARD_SIZE; file++){
            chess::Square s = chess::makeSquare(file, rank);
            bool dark = (rank + file) % 2 == 0;  // a1 is dark
            drawRect(renderer, squareRect(s, flipped), dark ? theme::DarkSquare : theme::LightSquare);
        }
    }

    if(!highlights.lastMove.isNull()){
        drawRect(renderer, squareRect(highlights.lastMove.from(), flipped), theme::LastMove);
        drawRect(renderer, squareRect(highlights.lastMove.to(), flipped), theme::LastMove);
    }
    if(highlights.selected != chess::NoSquare){
        drawRect(renderer, squareRect(highlights.selected, flipped), theme::Selected);
    }
    if(highlights.checkedKing != chess::NoSquare){
        drawShape(renderer, assets.glow, squareRect(highlights.checkedKing, flipped), theme::Check);
    }

    for(int rank = 0; rank < BOARD_SIZE; rank++){
        for(int file = 0; file < BOARD_SIZE; file++){
            chess::Square s = chess::makeSquare(file, rank);
            chess::Piece piece = board.pieceAt(s);
            if(piece == chess::NoPiece || s == highlights.dragged) continue;

            drawPiece(renderer, piece, squareRect(s, flipped), assets);
        }
    }

    drawCoordinates(renderer, assets.coordinateFont, flipped);

    for(chess::Square s : highlights.quietTargets) drawShape(renderer, assets.dot, squareRect(s, flipped), theme::MoveHint);
    for(chess::Square s : highlights.captureTargets) drawShape(renderer, assets.ring, squareRect(s, flipped), theme::MoveHint);
}

//  A straight arrow from the center of one square to the center of the other: a shaft and a
//  triangular head, built from triangles (SDL_RenderGeometry) so it can point any way and be
//  see-through without darker spots where the parts overlap

void drawArrow(SDL_Renderer* renderer, chess::Square from, chess::Square to, Color color, bool flipped){
    SDL_Rect a = squareRect(from, flipped), b = squareRect(to, flipped);
    float x0 = a.x + a.w / 2.0f, y0 = a.y + a.h / 2.0f;
    float x1 = b.x + b.w / 2.0f, y1 = b.y + b.h / 2.0f;

    float length = std::hypot(x1 - x0, y1 - y0);
    if(length < 1.0f) return;

    //  Unit vector along the arrow, and the one perpendicular to it
    float ux = (x1 - x0) / length, uy = (y1 - y0) / length;
    float px = -uy, py = ux;

    const float shaft = TILESIZE * 0.10f;  // half widths
    const float head = TILESIZE * 0.25f;
    const float headLength = TILESIZE * 0.40f;

    float bx = x1 - ux * headLength, by = y1 - uy * headLength;  // where the head starts

    SDL_Color c = {color.r, color.g, color.b, color.a};
    auto vertex = [&](float x, float y){ return SDL_Vertex{SDL_FPoint{x, y}, c, SDL_FPoint{0, 0}}; };

    SDL_Vertex vertices[7] = {
        vertex(x0 + px * shaft, y0 + py * shaft), vertex(x0 - px * shaft, y0 - py * shaft),
        vertex(bx - px * shaft, by - py * shaft), vertex(bx + px * shaft, by + py * shaft),
        vertex(bx + px * head, by + py * head), vertex(bx - px * head, by - py * head),
        vertex(x1, y1),
    };
    const int indices[9] = {0, 1, 2, 0, 2, 3, 4, 5, 6};
    SDL_RenderGeometry(renderer, nullptr, vertices, 7, indices, 9);
}

void drawButtons(SDL_Renderer* renderer, const std::vector<Button>& buttons, TTF_Font* font){
    for(const Button& button : buttons){
        if(!button.active) continue;

        if(!button.enabled){
            drawRect(renderer, button.rect, theme::ButtonDisabled);
            drawText(renderer, font, button.text, theme::ButtonTextDisabled, button.rect);
            continue;
        }
        drawRect(renderer, button.rect, button.hover ? button.colorHover : button.color);
        drawText(renderer, font, button.text, button.colorText, button.rect);
    }
}

void drawPiece(SDL_Renderer* renderer, chess::Piece piece, SDL_Rect rect, const Assets& assets){
    SDL_RenderCopy(renderer, assets.pieces[textureIndex(piece)], NULL, &rect);
}

void drawPromotion(SDL_Renderer* renderer, chess::Color side, Point mouse, const Assets& assets){
    SDL_Rect rect = promotionPickerRect();
    drawRect(renderer, rect, theme::PickerBackground);

    rect.w = TILESIZE;
    chess::PieceType hovered = promotionChoiceAt(mouse);

    for(chess::PieceType type : PromotionChoices){
        if(type == hovered) drawRect(renderer, rect, theme::PickerHover);
        drawPiece(renderer, chess::makePiece(side, type), rect, assets);
        rect.x += TILESIZE;
    }
}
