#include "game_screen.h"
#include "app.h"
#include "core/perft.h"
#include "def.h"
#include "theme.h"
#include <cstdio>
#include <iostream>

using namespace std;

//  The bot waits at least this long before playing, so its move does not appear at the same
//  instant as the player's and both can be followed
static const std::chrono::milliseconds MinimumBotDelay(400);

//  Right of the board
static const SDL_Rect PanelRect = {STARTING_POSX + TILESIZE * BOARD_SIZE + 30, STARTING_POSY, 240, 290};



GameScreen::GameScreen(App& app, const GameSetup& setup) : app(app), setup(setup){
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

bool GameScreen::isBotTurn() const{
    return setup.vsBot && board.sideToMove() != setup.humanColor;
}

bool GameScreen::flipped() const{
    return setup.vsBot && setup.humanColor == chess::Black;
}

//  Called after anything changes the position: recomputes the legal moves and the status,
//  drops the selection, and starts the bot if it is its turn

void GameScreen::onPositionChanged(){
    board.generateLegalMoves(legalMoves);
    gameStatus = board.status();
    selectedSquare = chess::NoSquare;
    draggedSquare = chess::NoSquare;
    pendingPromotion = chess::Move();

    //  Whatever the bot was thinking about is not the current position anymore
    bot.cancel();
    if(isBotTurn() && gameStatus == chess::GameStatus::Ongoing){
        bot.start(board, searchLimitsFor(setup.difficulty));
    }

    updateStatusText();
}

void GameScreen::updateStatusText(){
    bool whiteToMove = board.sideToMove() == chess::White;

    switch(gameStatus){
        case chess::GameStatus::Checkmate:
            if(setup.vsBot) statusText = isBotTurn() ? "Checkmate, you win" : "Checkmate, the bot wins";
            else statusText = whiteToMove ? "Checkmate, Black wins" : "Checkmate, White wins";
            break;
        case chess::GameStatus::Stalemate:
            statusText = "Draw by stalemate";
            break;
        case chess::GameStatus::FiftyMoveRule:
            statusText = "Draw by fifty move rule";
            break;
        case chess::GameStatus::ThreefoldRepetition:
            statusText = "Draw by repetition";
            break;
        case chess::GameStatus::InsufficientMaterial:
            statusText = "Draw, not enough material to mate";
            break;
        case chess::GameStatus::Ongoing:
            if(setup.vsBot) statusText = isBotTurn() ? "The bot is thinking..." : "Your turn";
            else statusText = whiteToMove ? "White to play" : "Black to play";
            break;
    }
}

void GameScreen::playMove(chess::Move m){
    board.makeMove(m);
    redoStack.clear();  // a new move starts a new line, the old one cannot be redone
    onPositionChanged();
}

//  Against the bot, undo and redo skip over the bot's moves so it is the player's turn again

void GameScreen::undoMove(){
    if(board.lastMove().isNull()) return;

    do{
        redoStack.push_back(board.lastMove());
        board.unmakeMove();
    } while(isBotTurn() && !board.lastMove().isNull());

    onPositionChanged();
}

void GameScreen::redoMove(){
    if(redoStack.empty()) return;

    do{
        board.makeMove(redoStack.back());
        redoStack.pop_back();
    } while(isBotTurn() && !redoStack.empty());

    onPositionChanged();
}

void GameScreen::newGame(){
    board = chess::Board();
    redoStack.clear();
    botHasPlayed = false;
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

bool GameScreen::canMove() const{
    return gameStatus == chess::GameStatus::Ongoing && !isBotTurn();
}

//  Plays the legal move from -> to, or opens the promotion picker if it is a promotion.
//  false if there is no such legal move.

bool GameScreen::tryMove(chess::Square from, chess::Square to){
    for(chess::Move m : legalMoves){
        if(m.from() != from || m.to() != to) continue;

        //  The four promotions share from and to, the picker decides which one is played
        if(m.promotion() != chess::NoPieceType) pendingPromotion = m;  // drawn in place of the status line
        else playMove(m);
        return true;
    }
    return false;
}

//  Everything happens when the button goes down, so the board answers at once. Pressing a target
//  of the selected piece plays the move. Pressing one of our pieces selects it and picks it up:
//  it follows the mouse until the button is released. Pressing anywhere else deselects.

void GameScreen::onBoardPressed(chess::Square s){
    if(!canMove()) return;
    if(selectedSquare != chess::NoSquare && tryMove(selectedSquare, s)) return;

    chess::Piece piece = s == chess::NoSquare ? chess::NoPiece : board.pieceAt(s);
    bool ownPiece = piece != chess::NoPiece && chess::colorOf(piece) == board.sideToMove();
    selectedSquare = ownPiece ? s : chess::NoSquare;
    draggedSquare = selectedSquare;
}

//  Dropping the piece on one of its targets plays the move. Anywhere else it goes back to its
//  square and stays selected, so it can still be moved with a click on the target.

void GameScreen::onBoardReleased(chess::Square s){
    chess::Square from = draggedSquare;
    draggedSquare = chess::NoSquare;

    if(from == chess::NoSquare || s == from) return;
    tryMove(from, s);
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
    h.dragged = draggedSquare;
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



// ---- Bot panel ----------------------------------------------------------------

//  1234567 -> "1,234,567"
static std::string withSeparators(uint64_t n){
    std::string digits = std::to_string(n);
    std::string result;
    for(size_t i = 0; i < digits.size(); i++){
        if(i > 0 && (digits.size() - i) % 3 == 0) result += ',';
        result += digits[i];
    }
    return result;
}

//  From white's point of view like most chess sites: +1.50 is good for white, -M3 means black mates in 3
static std::string formatEvaluation(int whiteScore){
    char text[32];
    if(chess::isMateScore(whiteScore)){
        int moves = chess::mateInMoves(whiteScore);
        std::snprintf(text, sizeof(text), "%cM%d", moves > 0 ? '+' : '-', moves > 0 ? moves : -moves);
    }
    else std::snprintf(text, sizeof(text), "%+.2f", whiteScore / 100.0);
    return text;
}

static std::string formatSpeed(uint64_t nodes, std::chrono::milliseconds elapsed){
    if(elapsed.count() <= 0) return "-";
    double perSecond = double(nodes) * 1000.0 / double(elapsed.count());

    char text[32];
    if(perSecond >= 1e6) std::snprintf(text, sizeof(text), "%.1fM /s", perSecond / 1e6);
    else std::snprintf(text, sizeof(text), "%.0fk /s", perSecond / 1e3);
    return text;
}

void GameScreen::drawBotPanel(SDL_Renderer* renderer, const Assets& assets) const{
    const int padding = 18;
    SDL_Rect panel = PanelRect;
    drawRect(renderer, panel, theme::Panel);

    int x = panel.x + padding, w = panel.w - 2 * padding;
    int y = panel.y + padding;

    chess::Color botColor = ~setup.humanColor;
    std::string details = std::string(botColor == chess::White ? "White" : "Black") + ", " + difficultyName(setup.difficulty);
    drawText(renderer, assets.buttonFont, "Bot", theme::Text, SDL_Rect{x, y, w, 26}, Align::Left);
    drawText(renderer, assets.infoFont, details, theme::PanelLabel, SDL_Rect{x, y, w, 26}, Align::Right);
    y += 34;

    //  While thinking, the progress of the running search, otherwise the search of the last move played
    bool thinking = bot.isThinking();
    chess::SearchResult shown = thinking ? bot.progress() : lastBotSearch;
    bool hasSearch = thinking || botHasPlayed;

    std::string state;
    if(thinking) state = "Thinking...";
    else if(botHasPlayed) state = "Played " + lastBotSearch.bestMove.toUci();
    else state = "Waiting for your move";
    drawText(renderer, assets.infoFont, state, theme::PanelLabel, SDL_Rect{x, y, w, 22}, Align::Left);
    y += 36;

    drawRect(renderer, SDL_Rect{x, y, w, 1}, theme::Button);
    y += 14;

    //  The score is from the bot's side, the panel shows it from white's
    int whiteScore = botColor == chess::White ? shown.score : -shown.score;
    bool hasIteration = hasSearch && shown.depth > 0;

    const std::pair<const char*, std::string> rows[] = {
        {"Best move", hasIteration ? shown.bestMove.toUci() : "-"},
        {"Evaluation", hasIteration ? formatEvaluation(whiteScore) : "-"},
        {"Depth", hasIteration ? std::to_string(shown.depth) : "-"},
        {"Positions", hasSearch ? withSeparators(shown.nodes) : "-"},
        {"Speed", hasSearch ? formatSpeed(shown.nodes, shown.elapsed) : "-"},
    };
    for(const auto& [label, value] : rows){
        drawText(renderer, assets.infoFont, label, theme::PanelLabel, SDL_Rect{x, y, w, 30}, Align::Left);
        drawText(renderer, assets.buttonFont, value, theme::Text, SDL_Rect{x, y, w, 30}, Align::Right);
        y += 34;
    }
}



// ---- Screen -------------------------------------------------------------------

void GameScreen::handleEvent(const SDL_Event& event){
    switch(event.type){
        case SDL_MOUSEBUTTONDOWN:
            if(event.button.button == SDL_BUTTON_LEFT){
                Point pressedAt = {event.button.x, event.button.y};

                if(!pendingPromotion.isNull()) onPromotionClicked(pressedAt);
                else if(!clickButtons(buttons, pressedAt)) onBoardPressed(squareAt(pressedAt, flipped()));
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if(event.button.button == SDL_BUTTON_LEFT){
                onBoardReleased(squareAt(Point{event.button.x, event.button.y}, flipped()));
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

    if(bot.isDone() && bot.elapsed() >= MinimumBotDelay){
        lastBotSearch = bot.takeResult();
        botHasPlayed = true;
        playMove(lastBotSearch.bestMove);
    }
}

void GameScreen::draw(SDL_Renderer* renderer){
    const Assets& assets = app.assets();

    drawBoard(renderer, board, highlights(), flipped(), assets);

    if(bot.isThinking()){
        chess::Move best = bot.progress().bestMove;
        if(!best.isNull()) drawArrow(renderer, best.from(), best.to(), theme::BotArrow, flipped());
    }

    drawButtons(renderer, buttons, assets.buttonFont);
    if(setup.vsBot) drawBotPanel(renderer, assets);

    if(!pendingPromotion.isNull()){
        drawPromotion(renderer, board.sideToMove(), mouse, assets);
    }
    else{
        SDL_Rect statusArea = {STARTING_POSX, 20, TILESIZE * BOARD_SIZE, 60};
        drawText(renderer, assets.titleFont, statusText, theme::Text, statusArea);
    }

    //  Last, so the piece in the hand is above everything
    if(draggedSquare != chess::NoSquare){
        SDL_Rect hand = {mouse.x - TILESIZE / 2, mouse.y - TILESIZE / 2, TILESIZE, TILESIZE};
        drawPiece(renderer, board.pieceAt(draggedSquare), hand, assets);
    }
}
