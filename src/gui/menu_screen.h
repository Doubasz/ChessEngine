#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include <vector>
#include "button.h"
#include "game_setup.h"
#include "screen.h"

class App;

//  The start menu, in two pages: the main page (play against a friend on the same computer,
//  against the bot, or quit) and the bot page (pick a color and a difficulty, then start)

class MenuScreen : public Screen{
    public:
        explicit MenuScreen(App& app);

        void handleEvent(const SDL_Event& event) override;
        void update(Point mouse) override;
        void draw(SDL_Renderer* renderer) override;

    private:
        enum class Page { Main, Bot };
        enum class ColorChoice { White, Black, Random };

        App& app;
        std::vector<Button> buttons;

        Page page = Page::Main;
        ColorChoice colorChoice = ColorChoice::White;
        Difficulty difficulty = Difficulty::Medium;

        //  Buttons are rebuilt from the state above at the start of the next update, never from
        //  inside a click: that would destroy the button whose onClick is running
        bool rebuildNeeded = true;

        void showPage(Page p);
        void buildMainPage();
        void buildBotPage();
        void startBotGame();
};

#endif
