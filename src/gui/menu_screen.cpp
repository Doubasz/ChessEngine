#include "menu_screen.h"
#include "app.h"
#include "def.h"
#include "theme.h"

static const int ButtonW = 320, ButtonH = 56, ButtonGap = 16;
static const int ButtonsY = 360;

static const SDL_Rect TitleArea = {0, 170, WIDTH, 90};
static const int KingSize = 90;
static const int KingGap = 150;  // from the center of the window to the inner edge of each king



MenuScreen::MenuScreen(App& app) : app(app){
    int x = (WIDTH - ButtonW) / 2;
    int y = ButtonsY;

    auto menuButton = [&](const std::string& text, std::function<void()> onClick){
        Button button = Button{x, y, ButtonW, ButtonH};
        button.name = text;
        button.text = text;
        button.onClick = onClick;
        y += ButtonH + ButtonGap;
        return button;
    };

    buttons.push_back(menuButton("Play vs friend", [this]() { this->app.startGame(); }));

    //  Enabled once the engine can search
    Button bot = menuButton("Play vs bot (coming soon)", nullptr);
    bot.enabled = false;
    buttons.push_back(bot);

    y += ButtonGap;  // a little apart from the ways to play
    buttons.push_back(menuButton("Quit", [this]() { this->app.quit(); }));
}



void MenuScreen::handleEvent(const SDL_Event& event){
    if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
        clickButtons(buttons, Point{event.button.x, event.button.y});
    }
    else if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE){
        app.quit();
    }
}

void MenuScreen::update(Point mouse){
    for(Button& button : buttons) button.updateHover(mouse);
}

void MenuScreen::draw(SDL_Renderer* renderer){
    const Assets& assets = app.assets();

    drawText(renderer, assets.logoFont, "Chess", theme::Text, TitleArea);

    //  A king on each side of the title
    int kingY = TitleArea.y + (TitleArea.h - KingSize) / 2;
    SDL_Rect whiteKing = {WIDTH / 2 - KingGap - KingSize, kingY, KingSize, KingSize};
    SDL_Rect blackKing = {WIDTH / 2 + KingGap, kingY, KingSize, KingSize};
    drawPiece(renderer, chess::makePiece(chess::White, chess::King), whiteKing, assets);
    drawPiece(renderer, chess::makePiece(chess::Black, chess::King), blackKing, assets);

    drawButtons(renderer, buttons, assets.buttonFont);
}
