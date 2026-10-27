#include "menu_screen.h"
#include "app.h"
#include "def.h"
#include "theme.h"
#include <random>

static const int ButtonW = 320, ButtonH = 56, ButtonGap = 16;
static const int ButtonsY = 360;

static const SDL_Rect TitleArea = {0, 170, WIDTH, 90};
static const int KingSize = 90;
static const int KingGap = 150;  // from the center of the window to the inner edge of each king

//  Bot page: two rows of options, each under its label
static const int OptionW = 150, OptionH = 50, OptionGap = 12;
static const int ColorLabelY = 230, DifficultyLabelY = 340;
static const int LabelH = 30;



MenuScreen::MenuScreen(App& app) : app(app){}

void MenuScreen::showPage(Page p){
    page = p;
    rebuildNeeded = true;
}

static Button makeButton(int x, int y, int w, int h, const std::string& text, std::function<void()> onClick){
    Button button = Button{x, y, w, h};
    button.name = text;
    button.text = text;
    button.onClick = onClick;
    return button;
}

void MenuScreen::buildMainPage(){
    int x = (WIDTH - ButtonW) / 2;
    int y = ButtonsY;

    auto next = [&](const std::string& text, std::function<void()> onClick){
        Button button = makeButton(x, y, ButtonW, ButtonH, text, onClick);
        y += ButtonH + ButtonGap;
        return button;
    };

    buttons.push_back(next("Play vs friend", [this]() { this->app.startGame(GameSetup()); }));
    buttons.push_back(next("Play vs bot", [this]() { showPage(Page::Bot); }));
    y += ButtonGap;  // a little apart from the ways to play
    buttons.push_back(next("Quit", [this]() { this->app.quit(); }));
}

void MenuScreen::buildBotPage(){
    //  A row of three options under a label, the chosen one drawn in the selected color
    auto optionRow = [&](int labelY, const char* const names[3], int chosen, std::function<void(int)> choose){
        int x = (WIDTH - (3 * OptionW + 2 * OptionGap)) / 2;
        for(int i = 0; i < 3; i++){
            Button option = makeButton(x, labelY + LabelH + 8, OptionW, OptionH, names[i], [choose, i]() { choose(i); });
            if(i == chosen){
                option.color = theme::ButtonSelected;
                option.colorHover = theme::ButtonSelectedHover;
            }
            buttons.push_back(option);
            x += OptionW + OptionGap;
        }
    };

    static const char* const colorNames[3] = {"White", "Black", "Random"};
    optionRow(ColorLabelY, colorNames, int(colorChoice), [this](int i){
        colorChoice = ColorChoice(i);
        rebuildNeeded = true;
    });

    static const char* const difficultyNames[3] = {
        difficultyName(Difficulty::Easy), difficultyName(Difficulty::Medium), difficultyName(Difficulty::Hard),
    };
    optionRow(DifficultyLabelY, difficultyNames, int(difficulty), [this](int i){
        difficulty = Difficulty(i);
        rebuildNeeded = true;
    });

    int x = (WIDTH - ButtonW) / 2;
    int y = DifficultyLabelY + LabelH + 8 + OptionH + 3 * ButtonGap;
    buttons.push_back(makeButton(x, y, ButtonW, ButtonH, "Start game", [this]() { startBotGame(); }));
    buttons.push_back(makeButton(x, y + ButtonH + ButtonGap, ButtonW, ButtonH, "Back", [this]() { showPage(Page::Main); }));
}

void MenuScreen::startBotGame(){
    GameSetup setup;
    setup.vsBot = true;
    setup.difficulty = difficulty;

    switch(colorChoice){
        case ColorChoice::White: setup.humanColor = chess::White; break;
        case ColorChoice::Black: setup.humanColor = chess::Black; break;
        case ColorChoice::Random: {
            std::random_device random;
            setup.humanColor = random() % 2 == 0 ? chess::White : chess::Black;
            break;
        }
    }
    app.startGame(setup);
}



void MenuScreen::handleEvent(const SDL_Event& event){
    if(event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT){
        clickButtons(buttons, Point{event.button.x, event.button.y});
    }
    else if(event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE){
        if(page == Page::Bot) showPage(Page::Main);
        else app.quit();
    }
}

void MenuScreen::update(Point mouse){
    if(rebuildNeeded){
        buttons.clear();
        if(page == Page::Main) buildMainPage();
        else buildBotPage();
        rebuildNeeded = false;
    }

    for(Button& button : buttons) button.updateHover(mouse);
}

void MenuScreen::draw(SDL_Renderer* renderer){
    const Assets& assets = app.assets();

    if(page == Page::Main){
        drawText(renderer, assets.logoFont, "Chess", theme::Text, TitleArea);

        //  A king on each side of the title
        int kingY = TitleArea.y + (TitleArea.h - KingSize) / 2;
        SDL_Rect whiteKing = {WIDTH / 2 - KingGap - KingSize, kingY, KingSize, KingSize};
        SDL_Rect blackKing = {WIDTH / 2 + KingGap, kingY, KingSize, KingSize};
        drawPiece(renderer, chess::makePiece(chess::White, chess::King), whiteKing, assets);
        drawPiece(renderer, chess::makePiece(chess::Black, chess::King), blackKing, assets);
    }
    else{
        drawText(renderer, assets.titleFont, "Play vs bot", theme::Text, SDL_Rect{0, 120, WIDTH, 60});
        drawText(renderer, assets.buttonFont, "Your color", theme::PanelLabel, SDL_Rect{0, ColorLabelY, WIDTH, LabelH});
        drawText(renderer, assets.buttonFont, "Difficulty", theme::PanelLabel, SDL_Rect{0, DifficultyLabelY, WIDTH, LabelH});
    }

    drawButtons(renderer, buttons, assets.buttonFont);
}
