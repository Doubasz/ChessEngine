#ifndef GAME_SCREEN_H
#define GAME_SCREEN_H

#include <string>
#include <vector>
#include "core/board.h"
#include "button.h"
#include "draw.h"
#include "screen.h"

class App;

//  A game between two players at the same computer, with undo/redo and a few debug tools

class GameScreen : public Screen{
    public:
        explicit GameScreen(App& app);

        void handleEvent(const SDL_Event& event) override;
        void update(Point mouse) override;
        void draw(SDL_Renderer* renderer) override;

    private:
        App& app;
        std::vector<Button> buttons;
        chess::Board board;

        //  Everything that depends on the position, recomputed by onPositionChanged after every change
        chess::MoveList legalMoves;
        chess::GameStatus gameStatus = chess::GameStatus::Ongoing;
        std::string statusText;  // "White to play", "Checkmate, Black wins"...

        chess::Square selectedSquare = chess::NoSquare;  // the piece the player clicked, NoSquare if none
        chess::Square pressedSquare = chess::NoSquare;  // where the mouse button went down
        Point mouse = {0, 0};

        //  Set while the promotion picker is shown: the pawn move waiting for its piece
        chess::Move pendingPromotion;

        //  Moves taken back with undo, the last one is replayed first by redo
        std::vector<chess::Move> redoStack;

        void onPositionChanged();
        void onSquareClicked(chess::Square s);
        void onPromotionClicked(Point mouse);
        void playMove(chess::Move m);
        void undoMove();
        void redoMove();
        void newGame();
        BoardHighlights highlights() const;

        //  Debug tools, they print to the console
        void printFen();
        void printMoves();
        void runPerft();
};

#endif
