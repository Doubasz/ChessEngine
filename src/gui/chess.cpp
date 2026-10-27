
#include "chess.h"
#include "core/perft.h"
#include <iostream>

using namespace std;



Chess::Chess(){
    int windowFlags = 0;
    int rendererFlags = SDL_RENDERER_ACCELERATED;

    if(SDL_Init(SDL_INIT_VIDEO) < 0){
        cout << "Couldn't init SDL" << endl;
        exit(1);
    }

    if (TTF_Init() == -1) {
        std::cerr << "TTF_Init failed: " << TTF_GetError() << std::endl;
        exit(1);
    }

    window = SDL_CreateWindow("Chess", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, windowFlags);
    renderer = SDL_CreateRenderer(window, -1, rendererFlags);

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);

    textures = loadAllTexture(renderer);

    font = TTF_OpenFont(FONT_PATH, 64);
    if (!font) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        exit(-1);
    }

    quit = false;

    int undoX = 538, undoY = 720, undoW = 50, undoH = 50;
    int redoX = undoX + undoW + 20;

    int stateX = 375, stateY = 20;
    int stateW = 450;
    int stateH = 60;

    Button stateGame = Button{stateX, stateY, stateW, stateH};
    stateGame.border = false;
    stateGame.name = "stateGame";
    stateGame.text = "";
    stateGame.color = Color(BG_COLOR);
    stateGame.colorHover = Color(BG_COLOR);
    stateGame.colorText = Color(0xFFFFFFFF);

    Button undo = Button(undoX, undoY, undoW, undoH);
    undo.name = "Undo";
    undo.text = "Undo";
    undo.onClick = [this]() { undoMove(); };

    Button redo = Button(redoX, undoY, undoW, undoH);
    redo.name = "Redo";
    redo.text = "Redo";
    redo.onClick = [this]() { redoMove(); };

    int leftButtonX = 50, leftButtonY = 250, leftButtonW = 210, leftButtonH = 70;

    Button newGame = Button{leftButtonX, leftButtonY, leftButtonW, leftButtonH};
    newGame.text = "New Game";
    newGame.name = "newGame";
    newGame.onClick = [this](){ runNewGame(); };

    int gap = 40;

    Button getFen = Button{leftButtonX, leftButtonY + gap + leftButtonH, leftButtonW, leftButtonH};
    getFen.text = "Get fen";
    getFen.name = "getFen";
    getFen.onClick = [this]() { this->getFen(); };

    Button perft = Button{940, 90, 215, 50};
    perft.text = "Run perft";
    perft.name = "perft";
    perft.onClick = [this]() { runPerft(); };

    Button prntMoves = Button{leftButtonX, leftButtonY + 40 + leftButtonH * 2, leftButtonW, leftButtonH};
    prntMoves.text = "Get List Moves";
    prntMoves.name = "prntMoves";
    prntMoves.onClick = [this]() { printMoves(); };

    buttons.push_back(undo);
    buttons.push_back(redo);
    buttons.push_back(stateGame);
    buttons.push_back(newGame);
    buttons.push_back(getFen);
    buttons.push_back(perft);
    buttons.push_back(prntMoves);

    onPositionChanged();
}

Chess::~Chess(){
    for(SDL_Texture* texture : textures){
        if(texture) SDL_DestroyTexture(texture);
    }
    if(font){
        TTF_CloseFont(font);
        font = nullptr;
    }
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    TTF_Quit();
    SDL_Quit();
}



// ---- Game actions -------------------------------------------------------------

//  Called after anything changes the position: recomputes the legal moves and the status,
//  drops the selection and updates the banner

void Chess::onPositionChanged(){
    board.generateLegalMoves(legalMoves);
    gameStatus = board.status();
    selectedSquare = chess::NoSquare;
    pendingPromotion = chess::Move();

    Button* stateGame = getButton("stateGame");
    if(stateGame == nullptr) return;

    stateGame->active = true;
    bool whiteToMove = board.sideToMove() == chess::White;

    switch(gameStatus){
        case chess::GameStatus::Checkmate:
            stateGame->text = whiteToMove ? "Checkmate, Black wins" : "Checkmate, White wins";
            break;
        case chess::GameStatus::Stalemate:
            stateGame->text = "Draw by stalemate";
            break;
        case chess::GameStatus::FiftyMoveRule:
            stateGame->text = "Draw by fifty move rule";
            break;
        case chess::GameStatus::Ongoing:
            stateGame->text = whiteToMove ? "White to play" : "Black to play";
            break;
    }
}

void Chess::playMove(chess::Move m){
    board.makeMove(m);
    redoStack.clear();  // a new move starts a new line, the old one cannot be redone
    onPositionChanged();
}

void Chess::undoMove(){
    chess::Move last = board.lastMove();
    if(last.isNull()) return;

    board.unmakeMove();
    redoStack.push_back(last);
    onPositionChanged();
}

void Chess::redoMove(){
    if(redoStack.empty()) return;

    board.makeMove(redoStack.back());
    redoStack.pop_back();
    onPositionChanged();
}

void Chess::runNewGame(){
    board = chess::Board();
    redoStack.clear();
    onPositionChanged();
}

void Chess::printMoves(){
    cout << "Moves: ";
    for(size_t i = 0; i < legalMoves.size(); i++){
        cout << legalMoves[i].toUci();
        if(i != legalMoves.size() - 1) cout << ", ";
    }
    cout << "\nnbMoves: " << legalMoves.size() << endl;
}

void Chess::getFen(){
    cout << board.fen() << endl;
}

void Chess::runPerft(){
    const int limit = 4;

    for(int depth = 1; depth <= limit; depth++){
        uint64_t nodes = chess::perft(board, depth);
        cout << "Depth " << depth << ": " << nodes << " nodes\n";
    }
}



// ---- Board input --------------------------------------------------------------

//  First click selects one of our pieces, second click on one of its targets plays the move.
//  Clicking another of our pieces selects it instead, clicking anywhere else deselects.

void Chess::onSquareClicked(chess::Square s){
    if(gameStatus != chess::GameStatus::Ongoing) return;  // only undo or new game once it is over

    if(selectedSquare != chess::NoSquare){
        for(chess::Move m : legalMoves){
            if(m.from() != selectedSquare || m.to() != s) continue;

            //  The four promotions share from and to, the picker decides which one is played
            if(m.promotion() != chess::NoPieceType){
                pendingPromotion = m;
                getButton("stateGame")->active = false;  // the picker is drawn over the banner
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

void Chess::onPromotionClicked(Point mouse){
    chess::PieceType choice = promotionChoiceAt(mouse);
    chess::Move pawnMove = pendingPromotion;

    pendingPromotion = chess::Move();
    getButton("stateGame")->active = true;

    if(choice == chess::NoPieceType) return;
    playMove(chess::Move(pawnMove.from(), pawnMove.to(), pawnMove.kind(), choice));
}

std::vector<chess::Square> Chess::highlightedSquares() const{
    std::vector<chess::Square> targets;
    if(selectedSquare == chess::NoSquare) return targets;

    for(chess::Move m : legalMoves){
        if(m.from() == selectedSquare) targets.push_back(m.to());
    }
    return targets;
}



// ---- Loop ---------------------------------------------------------------------

void Chess::draw(){
    drawBackground(renderer);
    drawBoard(renderer, highlightedSquares());
    drawPieces(renderer, board, textures);
    drawButtons(renderer, buttons, font);

    if(!pendingPromotion.isNull()){
        drawPromotion(renderer, board.sideToMove(), textures);
    }
    SDL_RenderPresent(renderer);
}

void Chess::gameLoop(){
    const int FPS = 60;
    const int frameDelay = 1000 / FPS;

    while(!quit){
        Uint32 frameStart = SDL_GetTicks();

        draw();
        doInput();

        //  Cap FPS
        int frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < frameDelay) {
            SDL_Delay(frameDelay - frameTime);
        }
    }
}

void Chess::doInput(){
    Point mouse;
    SDL_GetMouseState(&mouse.x, &mouse.y);

    for(Button& b: buttons){
        b.updateHover(mouse);
    }

    SDL_Event event;

    while(SDL_PollEvent(&event)){
        switch(event.type){
            case SDL_QUIT:
                quit = true;
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (event.button.button == SDL_BUTTON_LEFT){
                    Point pressedAt = {event.button.x, event.button.y};

                    if(pendingPromotion.isNull()) handleClick(pressedAt);
                    pressedSquare = squareAt(pressedAt);
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (event.button.button == SDL_BUTTON_LEFT){
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
                else if (event.key.keysym.sym == SDLK_RIGHT) redoMove();
                break;

            default:
                break;
        }
    }
}

void Chess::handleClick(Point mouse){
    for(const Button& button : buttons){
        if(button.isMouseOver(mouse) && button.onClick != nullptr){
            button.onClick();
        }
    }
}

Button* Chess::getButton(string name){
    for(Button& button : buttons){
        if(button.name == name){
            return &button;
        }
    }
    return nullptr;
}
