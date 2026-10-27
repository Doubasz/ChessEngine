#include "game_screen.h"
#include "app.h"
#include "core/perft.h"
#include "def.h"
#include "theme.h"
#include <iostream>

using namespace std;



GameScreen::GameScreen(App& app) : app(app){
    //  Undo and redo centered under the board
    int undoW = 110, undoH = 44, undoGap = 20;
    int undoX = STARTING_POSX + (TILESIZE * BOARD_SIZE) / 2 - undoW - undoGap / 2;
    int undoY = STARTING_POSY + TILESIZE * BOARD_SIZE + 20;
    int redoX = undoX + undoW + undoGap;

    Button undo = Button(undoX, undoY, undoW, undoH);
    undo.name = "Undo";
    undo.text = "Undo";
    undo.onClick = [this]() { undoMove(); };

    Button redo = Button(redoX, undoY, undoW, undoH);
    redo.name = "Redo";
    redo.text = "Redo";
    redo.onClick = [this]() { redoMove(); };

    //  One column left of the board, aligned with its top edge
    int sideX = 50, sideW = 210, sideH = 50, sideGap = 16;
    int sideY = STARTING_POSY;

    auto sideButton = [&](const std::string& name, const std::string& text, std::function<void()> onClick){
        Button button = Button{sideX, sideY, sideW, sideH};
        button.name = name;
        button.text = text;
        button.onClick = onClick;
        sideY += sideH + sideGap;
        return button;
    };

    buttons.push_back(undo);
    buttons.push_back(redo);
    buttons.push_back(sideButton("newGame", "New Game", [this]() { newGame(); }));
    buttons.push_back(sideButton("menu", "Main Menu", [this]() { this->app.showMenu(); }));

    sideY += sideH;  // the debug tools a little apart
    buttons.push_back(sideButton("getFen", "Get fen", [this]() { printFen(); }));
    buttons.push_back(sideButton("prntMoves", "Get List Moves", [this]() { printMoves(); }));
    buttons.push_back(sideButton("perft", "Run perft", [this]() { runPerft(); }));

    onPositionChanged();
}



// ---- Game actions -------------------------------------------------------------

//  Called after anything changes the position: recomputes the legal moves and the status,
//  drops the selection and updates the status line

void GameScreen::onPositionChanged(){
    board.generateLegalMoves(legalMoves);
    gameStatus = board.status();
    selectedSquare = chess::NoSquare;
    pendingPromotion = chess::Move();

    bool whiteToMove = board.sideToMove() == chess::White;

    switch(gameStatus){
        case chess::GameStatus::Checkmate:
            statusText = whiteToMove ? "Checkmate, Black wins" : "Checkmate, White wins";
            break;
        case chess::GameStatus::Stalemate:
            statusText = "Draw by stalemate";
            break;
        case chess::GameStatus::FiftyMoveRule:
            statusText = "Draw by fifty move rule";
            break;
        case chess::GameStatus::Ongoing:
            statusText = whiteToMove ? "White to play" : "Black to play";
            break;
    }
}

void GameScreen::playMove(chess::Move m){
    board.makeMove(m);
    redoStack.clear();  // a new move starts a new line, the old one cannot be redone
    onPositionChanged();
}

void GameScreen::undoMove(){
    chess::Move last = board.lastMove();
    if(last.isNull()) return;

    board.unmakeMove();
    redoStack.push_back(last);
    onPositionChanged();
}

void GameScreen::redoMove(){
    if(redoStack.empty()) return;

    board.makeMove(redoStack.back());
    redoStack.pop_back();
    onPositionChanged();
}

void GameScreen::newGame(){
    board = chess::Board();
    redoStack.clear();
    onPositionChanged();
}

void GameScreen::printMoves(){
    cout << "Moves: ";
    for(size_t i = 0; i < legalMoves.size(); i++){
        cout << legalMoves[i].toUci();
        if(i != legalMoves.size() - 1) cout << ", ";
    }
    cout << "\nnbMoves: " << legalMoves.size() << endl;
}

void GameScreen::printFen(){
    cout << board.fen() << endl;
}

void GameScreen::runPerft(){
    const int limit = 4;

    for(int depth = 1; depth <= limit; depth++){
        uint64_t nodes = chess::perft(board, depth);
        cout << "Depth " << depth << ": " << nodes << " nodes\n";
    }
}



// ---- Board input --------------------------------------------------------------

//  First click selects one of our pieces, second click on one of its targets plays the move.
//  Clicking another of our pieces selects it instead, clicking anywhere else deselects.

void GameScreen::onSquareClicked(chess::Square s){
    if(gameStatus != chess::GameStatus::Ongoing) return;  // only undo or new game once it is over

    if(selectedSquare != chess::NoSquare){
        for(chess::Move m : legalMoves){
            if(m.from() != selectedSquare || m.to() != s) continue;

            //  The four promotions share from and to, the picker decides which one is played
            if(m.promotion() != chess::NoPieceType){
                pendingPromotion = m;  // the picker is drawn in place of the status line
            }
            else playMove(m);
            return;
        }
    }

    chess::Piece piece = s == chess::NoSquare ? chess::NoPiece : board.pieceAt(s);
    bool ownPiece = piece != chess::NoPiece && chess::colorOf(piece) == board.sideToMove();
    selectedSquare = ownPiece ? s : chess::NoSquare;
}

//  A click on a piece of the picker plays the promotion with it, anywhere else cancels it

void GameScreen::onPromotionClicked(Point mouse){
    chess::PieceType choice = promotionChoiceAt(mouse);
    chess::Move pawnMove = pendingPromotion;

    pendingPromotion = chess::Move();

    if(choice == chess::NoPieceType) return;
    playMove(chess::Move(pawnMove.from(), pawnMove.to(), pawnMove.kind(), choice));
}

BoardHighlights GameScreen::highlights() const{
    BoardHighlights h;
    h.lastMove = board.lastMove();
    h.selected = selectedSquare;
    if(board.inCheck()) h.checkedKing = board.kingSquare(board.sideToMove());

    if(selectedSquare == chess::NoSquare) return h;

    for(chess::Move m : legalMoves){
        if(m.from() != selectedSquare) continue;

        //  The four promotions share a target, it is only added once
        if(m.promotion() != chess::NoPieceType && m.promotion() != chess::Queen) continue;

        bool capture = board.pieceAt(m.to()) != chess::NoPiece || m.kind() == chess::MoveKind::EnPassant;
        (capture ? h.captureTargets : h.quietTargets).push_back(m.to());
    }
    return h;
}



// ---- Screen -------------------------------------------------------------------

void GameScreen::handleEvent(const SDL_Event& event){
    switch(event.type){
        case SDL_MOUSEBUTTONDOWN:
            if(event.button.button == SDL_BUTTON_LEFT){
                Point pressedAt = {event.button.x, event.button.y};

                if(pendingPromotion.isNull()) clickButtons(buttons, pressedAt);
                pressedSquare = squareAt(pressedAt);
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if(event.button.button == SDL_BUTTON_LEFT){
                Point releasedAt = {event.button.x, event.button.y};

                if(!pendingPromotion.isNull()){
                    onPromotionClicked(releasedAt);
                }
                //  A click is a press and a release on the same square, dragging off cancels it
                else if(squareAt(releasedAt) == pressedSquare){
                    onSquareClicked(pressedSquare);
                }
                pressedSquare = chess::NoSquare;
            }
            break;

        case SDL_KEYDOWN:
            if(event.key.keysym.sym == SDLK_LEFT) undoMove();
            else if(event.key.keysym.sym == SDLK_RIGHT) redoMove();
            else if(event.key.keysym.sym == SDLK_ESCAPE) app.showMenu();
            break;

        default:
            break;
    }
}

void GameScreen::update(Point mouse){
    this->mouse = mouse;
    for(Button& b : buttons) b.updateHover(mouse);
}

void GameScreen::draw(SDL_Renderer* renderer){
    const Assets& assets = app.assets();

    drawBoard(renderer, board, highlights(), assets);
    drawButtons(renderer, buttons, assets.buttonFont);

    if(!pendingPromotion.isNull()){
        drawPromotion(renderer, board.sideToMove(), mouse, assets);
    }
    else{
        SDL_Rect statusArea = {STARTING_POSX, 20, TILESIZE * BOARD_SIZE, 60};
        drawText(renderer, assets.titleFont, statusText, theme::Text, statusArea);
    }
}
