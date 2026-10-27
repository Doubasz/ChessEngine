
#include "chess.h"
#include <iostream>

#include "bot.h"
#include "perft.h"

using namespace std;



bool mousePressed = false;
bool positionStored = false;
bool clicked = false;
bool isPawnPromotion = false;
bool choiceMade = false;

int choice;

Position selectPos;
Position piecePosInArray;
Tile* selectedPieceDnD;
Tile* selectedClickedPiece;
Tile* promotionPawn;
Position mousePos;

Position posClickedPieceInArray;
Position posClickedPiece;

Position promotionChoicePos;



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

    TTF_Font* fontInit = TTF_OpenFont(FONT_PATH, 64);
    if (!fontInit) {
        std::cerr << "Failed to load font: " << TTF_GetError() << std::endl;
        exit(-1);
    }

    font = fontInit;

    quit = false;

    board = Board();
    gameStatus = ONGOING;

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
    //undo.colorHover = Color(0x278c42);
    undo.name = "Undo";
    undo.text = "Undo";

    Board* b = &board;
    undo.onClick = [this]() { board.undoLastMove(); updateGameStatus(); };

    Button redo = Button(redoX, undoY, undoW, undoH);
    //redo.colorHover = Color(0xad953b);
    redo.name = "Redo";
    redo.text = "Redo";
    redo.onClick = [b]() { b->redoMove(); };

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
    //playBotRandom.onClick


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

    updateGameStatus();
    board.displayBoard();

}

void Chess::printMoves(){
    Move moves[256];

    int nbMoves = board.getAllMoves(moves, 256);

    cout << "Moves : ";
    for(int i = 0; i < nbMoves; i++){
        cout << moves[i].moveToAlbr();
        if(i != nbMoves - 1){
            cout << ", ";
        }
    }
    cout << endl;
    cout << "nbMoves: " << nbMoves << endl;
}

void Chess::getFen(){
    cout << board.posToFen() << endl;
}

void playBotRandom(){
    Bot botRandom = Bot();
}

void Chess::runNewGame(){
    board = Board();
    updateGameStatus();
}

//  Recomputes the status of the side to move and shows it in the stateGame banner

void Chess::updateGameStatus(){
    gameStatus = board.getGameStatus();

    Button* stateGame = getButton("stateGame");
    if(stateGame == nullptr) return;

    switch(gameStatus){
        case CHECKMATE:
            stateGame->text = (board.turnToMove == WHITE) ? "Checkmate, Black wins" : "Checkmate, White wins";
            break;
        case STALEMATE:
            stateGame->text = "Draw by stalemate";
            break;
        case DRAW_FIFTY_MOVES:
            stateGame->text = "Draw by fifty move rule";
            break;
        case ONGOING:
            stateGame->text = (board.turnToMove == WHITE) ? "White to play" : "Black to play";
            break;
    }
}

void undoLastMove(Board b){
    b.undoLastMove();
}

void redoMove(Board b){
    b.redoMove();
}

Chess::~Chess(){
    if (renderer) {
        SDL_DestroyRenderer(renderer);
        renderer = nullptr;
    }
    if (window) {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    if(font){
        TTF_CloseFont(font);
        font = nullptr;
    }
    SDL_Quit();
    TTF_Quit();
}

void Chess::draw(){
    drawBackground(renderer);
    drawBoard(renderer, board, selectedClickedPiece);
    drawPieces(renderer, board, textures);
    drawButtons(renderer, buttons, font);

    if(isPawnPromotion){
        drawPromotion(renderer, board, *promotionPawn, textures);
    }
    SDL_RenderPresent(renderer);
}




void Chess::gameLoop(){
    const int FPS = 60;
    const int frameDelay = 1000 / FPS; 

    Uint32 frameStart;
    int frameTime;

    while(!quit){
        frameStart = SDL_GetTicks();

        draw();
        doInput();
        

            // Cap FPS
        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < frameDelay) {
            SDL_Delay(frameDelay - frameTime);  // Wait for next frame
        }
    }
}

void Chess::doInput(){

    /*int undoX = 538, undoY = 720, undoW = 50, undoH = 50;
    int redoX = undoX + undoW + 20;

    Button undo = Button(undoX, undoY, undoW, undoH);
    Button redo = Button(redoX, undoY, undoW, undoH);*/

    Position mouse;
    SDL_GetMouseState(&mouse.x, &mouse.y);

    for(Button& b: buttons){
        b.updateHover(mouse);
    }

    

    SDL_Event event;

    while(SDL_PollEvent(&event)){


        switch(event.type){
            case SDL_QUIT:
                quit = true;
                exit(0);
                break;

            case SDL_MOUSEBUTTONDOWN:

                

                if (event.button.button == SDL_BUTTON_LEFT){

                    Position pressedAt = {event.button.x, event.button.y};


                    handleClick(pressedAt);

                    if(isPawnPromotion){

                        int x = (STARTING_POSX + (TILESIZE * BOARD)) / 2; // get the middle of the board (used it as a default value)
                        int y = STARTING_POSY - (20 + TILESIZE); // up the y of the board by 20, why 20? because it looks good

                        int width = (TILESIZE * 4); // 4 is the number of piece (q, b, n, r)
                        int height = TILESIZE;

                        SDL_Rect choiceRect = {x, y, width, height};

                        promotionChoicePos = pressedAt;
                        if(isBetweenZone(promotionChoicePos, choiceRect)){

                            // the choice is in the array [Q, R, B, N] (0-based)
                            choice = (promotionChoicePos.x / TILESIZE) - 6; // 6 big magic number
                            choiceMade = true;
                        } else{
                            choice = 0;
                            choiceMade = true;
                        }
                    }

                    if(isInBoard(pressedAt)){
                        posClickedPiece = pressedAt;
                        posClickedPieceInArray = getPiecePosInArray(posClickedPiece);
                        break;
                    }
                }
                
                





                /*if (event.button.button == SDL_BUTTON_LEFT && !mousePressed){
                    mousePressed = true;

                    if(!positionStored){
                        selectPos = Position(event.button.x, event.button.y);
                        positionStored = true;
                        cout << "x, y " << selectPos.x << " " << selectPos.y << endl;

                        if(isInBoard(selectPos)){

                            piecePosInArray = getPiecePosInArray(selectPos);
                            selectedPieceDnD = &board.tiles[piecePosInArray.x][piecePosInArray.y];

                            board.generateMoves(*selectedPieceDnD);


                            displayTab(selectedPieceDnD->legalMoves);

                            //displayMovesInBoard(board.tiles[piecePosInArray.x][piecePosInArray.y]);

                            
                        }
                    }
                }*/
                break;
                

            
            case SDL_MOUSEBUTTONUP:

                
                if (event.button.button == SDL_BUTTON_LEFT){

                    Button* stateGame = getButton("stateGame");
                    
                    Position releasedAt = {event.button.x, event.button.y};



                    if(choiceMade){

                        board.makePromotion(*promotionPawn, choice);
                        isPawnPromotion = false;
                        choiceMade = false;
                        promotionPawn = nullptr;

                        stateGame->active = true;

                        //  The status was computed with a queen, an underpromotion can change it
                        updateGameStatus();
                    }
                    
                    Position temp1 = getPiecePosInArray(releasedAt);

                    if(temp1 == posClickedPieceInArray){
                        //  Here we clicked and stayed in the same tile
                        cout << "temp: " << temp1.x << " " << temp1.y << endl;

                        Tile* clickPiece = &board.tiles[temp1.x][temp1.y];
                        cout << "Clicked piece is " << clickPiece->currentPos.x << ", " << clickPiece->currentPos.y << endl;

                        
                            //  Once the game is over no piece can be selected, only undo or new game
                            if(gameStatus != ONGOING){
                                selectedClickedPiece = nullptr;
                            }
                            else if(selectedClickedPiece == nullptr){

                                if(clickPiece->piece != NONE){
                                                                            
                                    if(clickPiece->color == board.turnToMove){
                                        
                                        selectedClickedPiece = clickPiece;
                                        board.generateLegalMoves(*selectedClickedPiece);
                                        cout << "Current selected piece is " << selectedClickedPiece->currentPos.x << " " << selectedClickedPiece->currentPos.y << endl;
                                        selectedClickedPiece->displayLegalMoves();

                                        cout << "Current selected piece is " << selectedClickedPiece->currentPos.x << " " << selectedClickedPiece->currentPos.y << endl;
                                    }
                                }
                            }
                            else{

                                

                                if(selectedClickedPiece->isInLegalMoves(clickPiece->currentPos)){
                                    Move temp = selectedClickedPiece->getInLegalMoves(clickPiece->currentPos);

                                    board.makeMove(temp);
                                    updateGameStatus();

                                    
                                    

                                    if(isPromotion(*clickPiece)){
                                        isPawnPromotion = true;
                                        promotionPawn = clickPiece;
                                        stateGame->active = false;
                                    }
                                    
                                    selectedClickedPiece = nullptr;

                                    displayTab(board.posOfPieces);
                                    board.displayBoard();

                                    cout << "Move completed" << endl;
                                }
                                else{
                                    
                                    if(clickPiece->color == board.turnToMove){
                                        selectedClickedPiece = clickPiece;
                                        board.generateLegalMoves(*selectedClickedPiece);
                                        selectedClickedPiece->displayLegalMoves();
                                        
                                    }
                                    else{
                                        selectedClickedPiece = nullptr;
                                        cout << "Piece deselected" << endl;
                                    }
                                    
                                }
                                
                            }    
                    }
                        
                    



                    /*if(isInBoard(Position(event.button.x, event.button.y))){

                        board.posTiles[piecePosInArray.x][piecePosInArray.y] = Position(STARTING_POSX + (TILESIZE * piecePosInArray.y), STARTING_POSY + (TILESIZE  * piecePosInArray.x));

                        displayPosTiles();
                        

                        if(board.tiles[piecePosInArray.x][piecePosInArray.y].piece != NONE){

                            cout << "x,y " << event.button.x << " " << event.button.y << endl;
                            Position targetSquare = getClosestTile(Position(event.button.x, event.button.y));

                            cout << targetSquare.x << " " << targetSquare.y << endl;
                            
                            if(!(piecePosInArray == targetSquare)){
                                board.tiles[targetSquare.x][targetSquare.y].setPiece(board.tiles[piecePosInArray.x][piecePosInArray.y].piece, board.tiles[piecePosInArray.x][piecePosInArray.y].color);
                                board.tiles[piecePosInArray.x][piecePosInArray.y].piece = NONE;
                            }

                            displayBoard();
                        }
                    }
                    else{
                        board.posTiles[piecePosInArray.x][piecePosInArray.y] = Position(STARTING_POSX + (TILESIZE * piecePosInArray.y), STARTING_POSY + (TILESIZE  * piecePosInArray.x));
                    }*/


                    mousePressed = false;
                    positionStored = false;
                    cout << "mouse released\n";
                }
                break;

            case SDL_KEYDOWN:
                if(event.key.keysym.sym == SDLK_LEFT){
                    //cout << "lastPos: " << board.lastPosition << endl;
                    cout << "left pressed" << endl;
                    
                    board.undoLastMove();
                    updateGameStatus();
                    board.displayBoard();
                }
                else if (event.key.keysym.sym == SDLK_RIGHT){

                    cout << "right pressed" << endl;

                    board.redoMove();
                    board.displayBoard();
                }
                
                break;

            default:
                break;
        }
    }
}

void getClick(Position pressed, Position released){

}

bool Chess::isButtonClicked(Position mouse){
    for(Button button : buttons){
        if(button.isMouseOver(mouse)){
            return true;
        }
    }
    return false;
}

void Chess::handleClick(Position mouse){
    for(Button button : buttons){
        if(button.isMouseOver(mouse)){
            if(button.onClick != nullptr){
                button.onClick();
            }
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


bool isInBoard(Position selectPos){
    return selectPos.x > STARTING_POSX && selectPos.y > STARTING_POSY && selectPos.x < STARTING_POSX + (TILESIZE * BOARD) && selectPos.y < STARTING_POSY + (TILESIZE * BOARD);
}

bool isPromotion(Tile target){
    return (target.currentPos.x == 7 || target.currentPos.x == 0) && target.piece == PAWN; 
}

bool isBetweenZone(Position posToVerify, SDL_Rect zone){
    return posToVerify.x >= zone.x && posToVerify.x <= zone.x + zone.w && posToVerify.y >= zone.y && posToVerify.y <= zone.y + zone.h;
}

//  Converts a pixel position in the window into [i][j] indexes of board.tiles, (-1, -1) if outside the board

Position Chess::getPiecePosInArray(Position posInWindow){
    if(!isInBoard(posInWindow)) return Position(-1, -1);

    return Position((posInWindow.y - STARTING_POSY) / TILESIZE, (posInWindow.x - STARTING_POSX) / TILESIZE);
}

void Chess::runPerft(){
    int limit = 2;

    /*uint64_t nodeSearched = dividePerft(limit);
    cout << "Depth " << limit << ": " << nodeSearched << " moves\n";*/

    for(int depth = 1; depth <= limit; depth++){
        uint64_t nodeSearched = dividePerft(board, depth, cout);
        cout << "Depth " << depth << ": " << nodeSearched << " moves\n";
    }
}
