#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <string>
#include <vector>
#include "core/board.h"
#include "bot_player.h"
#include "button.h"
#include "draw.h"
#include "game_setup.h"
#include "screen.h"

class App;

//  A game between two players at the same computer, or against the bot, with undo/redo and a
//  few debug tools. Against the bot a panel next to the board shows what it is thinking.

class GameScreen : public Screen{
    public:
        GameScreen(App& app, const GameSetup& setup);

        void handleEvent(const SDL_Event& event) override;
        void update(Point mouse) override;
        void draw(SDL_Renderer* renderer) override;

    private:
        App& app;
        GameSetup setup;
        std::vector<Button> buttons;
        chess::Board board;

        //  Everything that depends on the position, recomputed by onPositionChanged after every change
        chess::MoveList legalMoves;
        chess::GameStatus gameStatus = chess::GameStatus::Ongoing;
        std::string statusText;  // "White to play", "Checkmate, Black wins"...

        chess::Square selectedSquare = chess::NoSquare;  // the piece the player clicked, NoSquare if none
        chess::Square draggedSquare = chess::NoSquare;  // the piece held with the mouse button down, NoSquare if none
        Point mouse = {0, 0};

        //  Set while the promotion picker is shown: the pawn move waiting for its piece
        chess::Move pendingPromotion;

        //  Moves taken back with undo, the last one is replayed first by redo
        std::vector<chess::Move> redoStack;

        BotPlayer bot;
        chess::SearchResult lastBotSearch;  // of the move the bot played last, shown while the player thinks
        bool botHasPlayed = false;

        bool isBotTurn() const;
        bool flipped() const;  // the board is seen from black's side when the player has black

        void onPositionChanged();
        void updateStatusText();
        bool canMove() const;  // the player may move a piece now
        bool tryMove(chess::Square from, chess::Square to);
        void onBoardPressed(chess::Square s);
        void onBoardReleased(chess::Square s);
        void onPromotionClicked(Point mouse);
        void playMove(chess::Move m);
        void undoMove();
        void redoMove();
        void newGame();
        BoardHighlights highlights() const;

        void drawBotPanel(SDL_Renderer* renderer, const Assets& assets) const;

        //  Debug tools, they print to the console
        void printFen();
        void printMoves();
        void runPerft();
};

#endif
