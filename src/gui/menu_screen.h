#ifndef MENU_SCREEN_H
#define MENU_SCREEN_H

#include <vector>
#include "button.h"
#include "screen.h"

class App;

//  The start menu: play against a friend on the same computer, against the bot, or quit

class MenuScreen : public Screen{
    public:
        explicit MenuScreen(App& app);

        void handleEvent(const SDL_Event& event) override;
        void update(Point mouse) override;
        void draw(SDL_Renderer* renderer) override;

    private:
        App& app;
        std::vector<Button> buttons;
};

#endif
