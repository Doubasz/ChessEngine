#include "board.h"
#include "tile.h"
#include "def.h"
#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <unordered_map>

using namespace std;



Board::Board(){

    //  Using FEN we can write any position and compute it easily 

    string startingPosition = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    //string startingPosition = "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1";
    //string startingPosition = "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1";
    //string startingPosition = "R6R/3Q4/1Q4Q1/4Q3/2Q4Q/Q4Q2/pp1Q4/kBNN1KB1 w - - 0 1";

    
    parseFENPosition(startingPosition);

    //  Here I'm initialising the position in the window of the board, used in drag and drop (although not used now)

    Position startingPos = Position(STARTING_POSX, STARTING_POSY);

    for(int i = 0; i < BOARD; i++){
        startingPos.x = STARTING_POSX;

        for(int j = 0; j < BOARD; j++){
            posTiles[i][j] = startingPos;
            tiles[i][j].currentPos = {i, j};

            startingPos.x += TILESIZE;
        }
        startingPos.y += TILESIZE;
    }


}


Board::Board(const Board& other) {
    // 1. Copy `tiles` and `posTiles`
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j) {
            tiles[i][j] = other.tiles[i][j];
            posTiles[i][j] = other.posTiles[i][j];
        }
    }

    // 2. Copy vectors
    posOfPieces = other.posOfPieces;

    // 3. Copy strings and simple attributes
    lastMove = other.lastMove;
    nbMoves = other.nbMoves;
    turnToMove = other.turnToMove;
    nbHalfMoves = other.nbHalfMoves;

    // 4. enPassant
    enPassantActive = other.enPassantActive;
    enPassantTarget = other.enPassantTarget;

/*
    // 5. Copy `textures`
    if (other.textures) {
        textures = new SDL_Texture*[13]; // Assuming 13 textures
        for (int i = 0; i < 13; ++i) {
            textures[i] = other.textures[i]; // Shallow copy (adjust for deep copy if needed)
        }
    } else {
        textures = nullptr;
    }
*/
}

//  Insha allah no memory leaks

Board::~Board(){}



void Board::changeTurn(){
    turnToMove = (turnToMove == WHITE) ? BLACK : WHITE;
}


















std::string Board::posToFen(){
    string fen = "";

    for(int i = 0; i < BOARD; i++){
        int emptyCount = 0;
        for(int j = 0; j < BOARD; j++){
            Tile t = tiles[i][j];
            if(t.piece == NONE){
                emptyCount++;
            } else{
                if(emptyCount > 0){
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                fen += pieceToFen(t);
            }
        }
        if(emptyCount > 0) fen += std::to_string(emptyCount);
        if (i < 7) fen += '/';
    }

    //  Color to move
    fen += " ";
    fen += (turnToMove == WHITE) ? 'w' : 'b';

    //  Castling rights
    fen += " ";

    if(tiles[7][4].neverMoved){
        if(tiles[7][7].neverMoved)fen += 'K';
        if(tiles[7][0].neverMoved) fen += 'Q';
    }
    if(tiles[0][4].neverMoved){
        if(tiles[0][7].neverMoved) fen += 'k';
        if(tiles[0][0].neverMoved) fen += 'q';
    }

    if(!(tiles[7][4].neverMoved || tiles[0][4].neverMoved)){
        fen += "-";
    }

    //  En passant
    fen += " ";

    if(enPassantActive){
        fen += toAlgebraic(enPassantTarget);
    }else{
        fen += "-";
    }
    
    
    //  HalfMoves
    fen += " ";
    fen += std::to_string(nbHalfMoves);

    //  Moves
    fen += " ";
    fen += std::to_string(nbMoves);

    return fen;
}



char pieceToFen(Tile t){
    int color = (t.color == WHITE) ? 0 : 32;
    switch (t.piece){
        case PAWN:   return 'P' + color;
        case KNIGHT: return 'N' + color;
        case BISHOP: return 'B' + color;
        case ROOK:   return 'R' + color;
        case QUEEN:  return 'Q' + color;
        case KING:   return 'K' + color;
        default:     return '.';
    }
}



//  This initialise the piece array and their positions in board depending on the starting fen pos

void Board::parseFENPosition(string position){

    posOfPieces.clear(); 


    std::unordered_map<char, std::pair<int, int>> pieceMap = {
        {'p', {PAWN, BLACK}}, {'n', {KNIGHT, BLACK}}, {'b', {BISHOP, BLACK}}, {'r', {ROOK, BLACK}},
        {'q', {QUEEN, BLACK}}, {'k', {KING, BLACK}}, {'P', {PAWN, WHITE}}, {'N', {KNIGHT, WHITE}},
        {'B', {BISHOP, WHITE}}, {'R', {ROOK, WHITE}}, {'Q', {QUEEN, WHITE}}, {'K', {KING, WHITE}}
    };

    vector<string> fenPosition;
    fenPosition = split(position, ' ');

    if(fenPosition.size() < 6){
        std::cout << "Something went wrong in the split in fenToPosition\n";
        exit(1);
    }


    string realPosition = fenPosition[0];

    unsigned int iTab = 0, jTab = 0;

    for(char ch : realPosition){

        if(pieceMap.find(ch) != pieceMap.end()){

            //  affect these two variables with  the value in the hashmap
            auto [pieceType, color] = pieceMap[ch];

            tiles[iTab][jTab] = Tile(pieceType, color);
            jTab++;
        }

        //  When reaching '/' we go to the next array in [i][j]
        else if(ch == '/') {
            iTab++;
            jTab = 0;
        }

        //  When reaching a digit that means the number of empty tile tile the next piece are equal to the number
        else if (isdigit(ch)){
            int step = ch - '0';

            for(int k = 0; k < step; k++){
                tiles[iTab][jTab].piece = NONE;
                tiles[iTab][jTab].color = -1;
                jTab++;
            }
        }
    }

    for(int i = 0; i < BOARD; i++){
        for(int j = 0; j < BOARD; j++){
            if(tiles[i][j].piece != NONE){
                posOfPieces.push_back({i, j});
            }
            tiles[i][j].currentPos = {i, j};
        }
    }

    //  Setting the pawns neverMoved
    for(int i = 0; i < 8; i++){
        tiles[1][i].neverMoved = (tiles[1][i].piece == PAWN) ? true : false;
        tiles[6][i].neverMoved = (tiles[6][i].piece == PAWN) ? true : false;
    }

    //  if 'w' white to move and vice versa
    turnToMove = (fenPosition[1] == "w") ? WHITE : BLACK;

    //  Castling Rights

    tiles[7][4].neverMoved = (fenPosition[2].find('K') != string::npos || (fenPosition[2].find('Q') != string::npos)) ? true : false;
    tiles[0][4].neverMoved = (fenPosition[2].find('k') != string::npos || (fenPosition[2].find('q') != string::npos)) ? true : false;

    tiles[7][7].neverMoved = (fenPosition[2].find('K') != string::npos) ? true : false;
    tiles[7][0].neverMoved = (fenPosition[2].find('Q') != string::npos) ? true : false;
    tiles[0][7].neverMoved = (fenPosition[2].find('k') != string::npos) ? true : false;
    tiles[0][0].neverMoved = (fenPosition[2].find('q') != string::npos) ? true : false;

    //  En passant
    enPassantActive = (fenPosition[3].find('-') != string::npos) ? false : true;

    if(enPassantActive){
        enPassantTarget = translateAlgebraic(fenPosition[3]);
    }
    else{
        enPassantTarget = {-1, -1};
    }


    nbHalfMoves = std::stoi(fenPosition[4]);
    nbMoves = std::stoi(fenPosition[5]);

}

//  basically e4 -> 4, 4 

Position translateAlgebraic(string s){
    if(s.length() != 2){
        cout << "A problem in the en passant of the fen" << endl;
    }

    int i = s[0] - 'a';
    int j = abs(s[1] - '8');
    return {j, i};
}




std::vector<std::string> split(const std::string& str, char delimiter) {

    //  Split a string and return a vector of strings

    std::vector<std::string> tokens;
    std::stringstream ss(str);
    std::string token;

    while (std::getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}


















void Board::generateAllMoves(){
    //  Init all moves in the board, currently not used anywhere I hope I wont need it

    for(int i = 0; i < BOARD; i++){
        for(int j = 0; j < BOARD; j++){
            if(tiles[i][j].piece != NONE){
                generateMoves(tiles[i][j]);
            }
        }
    }
}



void Board::generateLegalMoves(Tile& tile){
    tile.legalMoves.clear();

    if(tile.color == turnToMove){
        generateMoves(tile);
        tile.legalMoves = filterLegalMoves(tile);
    }
    
}


//  this function init the moves of a Tile object in the tiles[i][j] attribute of a board
//  It store the position (i, j) of the array (tiles) into the legalMoves vector attribute of the tile

void Board::generateMoves(Tile& tile){

    //  Clearing the array of moves to avoid unwanted problems

    tile.legalMoves.clear();

    Position startingSquare = tile.currentPos;
    vector<Move> targetSquares;

    
    //  Lambda functions used to init the moves of a Pawn

    //  Init the moves of a pawn moving Forward

    //  Capturing Moves of a Pawn


    switch(tile.piece){

        case PAWN:{

            //  Depends on the color 
            int direction = (tile.color == WHITE) ? -1 : 1;

            //  Pretty straightforward

            Position forward = {startingSquare.x + direction, startingSquare.y};

            if(notOutOfBonds(forward) && tiles[forward.x][forward.y].piece == NONE){
                if(forward.x == 7 || forward.x == 0){
                    targetSquares.push_back({Q_PROMOTION, startingSquare, forward});
                    targetSquares.push_back({R_PROMOTION, startingSquare, forward});
                    targetSquares.push_back({B_PROMOTION, startingSquare, forward});
                    targetSquares.push_back({N_PROMOTION, startingSquare, forward});
                }
                else{
                    targetSquares.push_back({QUIET, startingSquare, forward});
                }
                

                //  if the pawn never moved he can move up 2 squares
                if(tiles[forward.x + direction][forward.y].piece == NONE && tile.neverMoved){
                    targetSquares.push_back({D_PAWN_PUSH, startingSquare, {forward.x + direction, forward.y}});
                }
            }


            Position captureLeft = {startingSquare.x + direction, startingSquare.y - 1};
            Position captureRight = {startingSquare.x + direction, startingSquare.y + 1};

            if(captureLeft == enPassantTarget && tile.isEnnemy(tiles[captureLeft.x - direction][captureLeft.y])) targetSquares.push_back({EP_CAPTURE, startingSquare, captureLeft});
            if(captureRight == enPassantTarget && tile.isEnnemy(tiles[captureRight.x - direction][captureRight.y])) targetSquares.push_back({EP_CAPTURE, startingSquare, captureRight});


            if(notOutOfBonds(captureLeft) && tile.isEnnemy(tiles[captureLeft.x][captureLeft.y])){
                if(captureLeft.x == 7 || captureLeft.x == 0){
                    targetSquares.push_back({Q_PROMOTION_CAPTURE, startingSquare, captureLeft, tiles[captureLeft.x][captureLeft.y].piece});
                    targetSquares.push_back({R_PROMOTION_CAPTURE, startingSquare, captureLeft, tiles[captureLeft.x][captureLeft.y].piece});
                    targetSquares.push_back({B_PROMOTION_CAPTURE, startingSquare, captureLeft, tiles[captureLeft.x][captureLeft.y].piece});
                    targetSquares.push_back({N_PROMOTION_CAPTURE, startingSquare, captureLeft, tiles[captureLeft.x][captureLeft.y].piece});
                }
                else targetSquares.push_back({CAPTURE, startingSquare, captureLeft, tiles[captureLeft.x][captureLeft.y].piece});
            }

            if(notOutOfBonds(captureRight) && tile.isEnnemy(tiles[captureRight.x][captureRight.y])){
                if(captureRight.x == 7 || captureRight.x == 0){
                    targetSquares.push_back({Q_PROMOTION_CAPTURE, startingSquare, captureRight, tiles[captureRight.x][captureRight.y].piece});
                    targetSquares.push_back({R_PROMOTION_CAPTURE, startingSquare, captureRight, tiles[captureRight.x][captureRight.y].piece});
                    targetSquares.push_back({B_PROMOTION_CAPTURE, startingSquare, captureRight, tiles[captureRight.x][captureRight.y].piece});
                    targetSquares.push_back({N_PROMOTION_CAPTURE, startingSquare, captureRight, tiles[captureRight.x][captureRight.y].piece});
                }
                else targetSquares.push_back({CAPTURE, startingSquare, captureRight, tiles[captureRight.x][captureRight.y].piece});
            }

            break;
        }
        case KNIGHT:{

            //  Init the moves of Knight for now its the only thing that works correctly

            const Position directions[8] = {{-2, -1}, {-2, 1}, {2, -1}, {2, 1}, {-1, -2}, {-1, 2}, {1, -2}, {1, 2}};

            for(int i = 0; i < 8; i++){
                Position pos = {startingSquare.x + directions[i].x, startingSquare.y + directions[i].y};

                if(notOutOfBonds(pos)){
                    if(tile.isEnnemy(tiles[pos.x][pos.y])){
                        targetSquares.push_back({CAPTURE, startingSquare, pos, tiles[pos.x][pos.y].piece});
                    }
                    else if (tiles[pos.x][pos.y].piece == NONE){
                        targetSquares.push_back({QUIET, startingSquare, pos});
                    }
                }
            }
            break;
        }
        case BISHOP:
        case ROOK:
        case QUEEN:{

            //  Init the moves of Sliding pieces
            //  TODO the moves of the king are almost the same just with more conditions and castle

            const vector<Position> directions = getDirectionSlidingPiece(tile);
            

            for(const Position& dir : directions){
                Position moving = {startingSquare.x , startingSquare.y};
                
                while(true){

                    moving.x += dir.x;
                    moving.y += dir.y;

                    if(!notOutOfBonds(moving)) break;

                    if(!tile.notBlockedBy(tiles[moving.x][moving.y])) break;
                    if(tile.isEnnemy(tiles[moving.x][moving.y])){
                        targetSquares.push_back({CAPTURE, startingSquare, moving, tiles[moving.x][moving.y].piece});
                        break;
                    }

                    targetSquares.push_back({QUIET, startingSquare, moving});
                }
            }
            break;
        }


        case KING:{
            Position directions[8] = {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};

            for(int i = 0; i < 8; i++){
                Position moving = {startingSquare.x + directions[i].x, startingSquare.y + directions[i].y};

                if(notOutOfBonds(moving)){
                    if(tiles[moving.x][moving.y].piece == NONE){
                        targetSquares.push_back({QUIET, startingSquare, moving});
                    }
                    else if(tile.isEnnemy(tiles[moving.x][moving.y])){
                        targetSquares.push_back({CAPTURE, startingSquare, moving, tiles[moving.x][moving.y].piece});
                    }
                }
                    
            }

            tile.legalMoves = targetSquares;

            if(tile.neverMoved){
                tile.legalMoves = filterLegalMoves(tile);

                int direction = 2;

                Position castleKingSide = {startingSquare.x, startingSquare.y + direction};
                Position castleQueenSide = {startingSquare.x, startingSquare.y - direction};

                
                if(tile.notBlockedBy(tiles[castleKingSide.x][castleKingSide.y])){

                    if(tile.isInLegalMoves({QUIET, startingSquare, {castleKingSide.x, castleKingSide.y - 1}}) && tiles[castleKingSide.x][castleKingSide.y + 1].neverMoved){

                        if(!isKingInCheck(tile.color)){
                            tile.legalMoves.push_back({KCASTLE, startingSquare, castleKingSide});
                        }
                    }
                }
                if (tile.notBlockedBy(tiles[castleQueenSide.x][castleQueenSide.y])){
                    if(tile.isInLegalMoves({QUIET, startingSquare, {castleQueenSide.x, castleQueenSide.y + 1}}) && tiles[castleQueenSide.x][castleQueenSide.y - 2].neverMoved){
                        if(!isKingInCheck(tile.color)){
                            tile.legalMoves.push_back({QCASTLE, startingSquare, castleQueenSide});
                        }
                    }
                }
            }
            


            break;
        }
        default:
            break;
    }

    //  Ugly but works
    if(tile.piece != KING){
        tile.legalMoves = targetSquares;
    }
    
}



//  Out of Bonds 0 =< p < 8

bool notOutOfBonds(Position p){
    return p.x < BOARD && p.x >= 0 && p.y < BOARD && p.y >= 0;
}


//  return  the pos (forgot where its used) surely gonna delete it 

Position getTileTillOutBonds(Position p){
    return {BOARD - p.x, BOARD - p.y};
}


//  Return the directions in which a sliding piece moves

vector<Position> getDirectionSlidingPiece(Tile t){
    switch(t.piece){
        case BISHOP:
            return {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}};
        case ROOK:
            return {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        case QUEEN:
            return {{-1, -1}, {-1, 1}, {1, -1}, {1, 1}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}};

    }
    return {{0, 0}};
}


















//  This function is used to make a move in the board, the things that are changing are the board.tiles and board.posPiece
//  the tiles used as parameter, I choose to parse by reference to change board.tiles, it basically swap two tiles when making a move, or create a whole new tile with the piece needed

void Board::tryMakeMove(Move move){

    Tile& start = tiles[move.start.x][move.start.y];
    Tile& target = tiles[move.target.x][move.target.y];

    switch(move.type){
        case QUIET:{
            target.setPiece(start);
            start.setPiece(NONE, -1);
            std::replace(posOfPieces.begin(), posOfPieces.end(), start.currentPos, target.currentPos); 
            break;
        }
        case D_PAWN_PUSH:{
            int direction = (start.color == WHITE) ? -1 : 1;
            
            target.setPiece(start);
            start.setPiece(NONE, -1);
            std::replace(posOfPieces.begin(), posOfPieces.end(), start.currentPos, target.currentPos);
            
            enPassantActive = true;
            enPassantTarget = {start.currentPos.x + direction, start.currentPos.y};

            break;
        }
    
        case KCASTLE:
        case QCASTLE:{

            int rookPosY = ((target.currentPos.y - start.currentPos.y) > 0) ? 1 : -2;
            int rookNewPos = (rookPosY > 0) ? -1 : 1;

            tiles[target.currentPos.x][target.currentPos.y + rookPosY].setPiece(NONE, -1);
            tiles[target.currentPos.x][target.currentPos.y + rookNewPos].setPiece(ROOK, start.color);

            target.setPiece(start);
            start.setPiece(NONE, -1);

            std::replace(posOfPieces.begin(), posOfPieces.end(), tiles[target.currentPos.x][target.currentPos.y + rookPosY].currentPos, tiles[target.currentPos.x][target.currentPos.y + rookNewPos].currentPos);
            std::replace(posOfPieces.begin(), posOfPieces.end(), start.currentPos, target.currentPos);

            break;
        }
        
        case CAPTURE:{
            posOfPieces.erase(std::remove(posOfPieces.begin(), posOfPieces.end(), target.currentPos), posOfPieces.end());
            std::replace(posOfPieces.begin(), posOfPieces.end(), start.currentPos, target.currentPos);

            target.setPiece(start);
            start.setPiece(NONE, -1);  

            break;
        }

        case EP_CAPTURE:{
            int direction = (start.color == WHITE) ? -1 : 1;

            std::replace(posOfPieces.begin(), posOfPieces.end(), start.currentPos, target.currentPos);
            posOfPieces.erase(std::remove(posOfPieces.begin(), posOfPieces.end(), tiles[target.currentPos.x - direction][target.currentPos.y].currentPos), posOfPieces.end());

            target.setPiece(start);
            tiles[target.currentPos.x - direction][target.currentPos.y].setPiece(NONE, -1);
            start.setPiece(NONE, -1);

            enPassantActive = false;
            enPassantTarget = {-1, -1};

            break;

        }
        case N_PROMOTION:
        case B_PROMOTION:
        case R_PROMOTION:
        case Q_PROMOTION:{
            
            break;
        }
        case N_PROMOTION_CAPTURE:
        case B_PROMOTION_CAPTURE:
        case R_PROMOTION_CAPTURE:
        case Q_PROMOTION_CAPTURE:{
            break;
        }
    }

    if(enPassantActive && move.type != D_PAWN_PUSH){
        enPassantActive = false;
        enPassantTarget = {-1, -1};
    }
}


void Board::makePromotion(Tile& target, int choice){

        int pieceChoosed = (choice == 0) ? QUEEN : (choice == 1) ? ROOK : (choice == 2) ? BISHOP : (choice == 3) ? KNIGHT : NONE;
        cout << "pieceChoosed: " << pieceChoosed << endl;

        target.setPiece(pieceChoosed, target.color);
    
}


//  Splited this concept into 2 different functions to use the one that really make moves in the filter legalMoves function
//  get the pos of the game to maybe in the future write it into a file that contains all the moves of the game

void Board::makeMove(Move move){
    //int turn = tiles[move.start.x][move.start.y].color;

        lastMove = move;
        //undoStack.push(lastPosition);
        
        tryMakeMove(move);

        //while(!redoStack.empty()) redoStack.pop();

        //std::replace(posOfPieces.begin(), posOfPieces.end(), start.currentPos, target.currentPos);
        changeTurn();
}



void Board::undoLastMove(){
    /*if(!undoStack.empty()){
        string lastMove = undoStack.top();
        redoStack.push(posToFen());

        parseFENPosition(lastMove);
        undoStack.pop();
    }*/
    Move nullMove = {-1, {-1, -1}, {-1, -1}};
    if (!(lastMove == nullMove)){
        undoMove(lastMove);
        lastMove = nullMove;
    }
    
}

void Board::undoMove(Move move){

    Tile& start = tiles[move.start.x][move.start.y];
    Tile& target = tiles[move.target.x][move.target.y];

    switch(move.type){
        case QUIET:{
            start.setPiece(target);
            target.setPiece(NONE, -1);
            std::replace(posOfPieces.begin(), posOfPieces.end(), target.currentPos, start.currentPos); 
            break;
        }
        case D_PAWN_PUSH:{

            
            start.setPiece(target);
            start.neverMoved = true;

            target.setPiece(NONE, -1);
            std::replace(posOfPieces.begin(), posOfPieces.end(), target.currentPos, start.currentPos);
            
            enPassantActive = false;
            enPassantTarget = {-1, -1};

            break;
        }
    
        case KCASTLE:
        case QCASTLE:{

            int rookPosY = ((target.currentPos.y - start.currentPos.y) > 0) ? 1 : -2;
            int rookNewPos = (rookPosY > 0) ? -1 : 1;

            tiles[target.currentPos.x][target.currentPos.y + rookPosY].setPiece(ROOK, target.color);
            tiles[target.currentPos.x][target.currentPos.y + rookNewPos].setPiece(NONE, -1);

            start.setPiece(target);
            target.setPiece(NONE, -1);

            std::replace(posOfPieces.begin(), posOfPieces.end(), tiles[target.currentPos.x][target.currentPos.y + rookNewPos].currentPos, tiles[target.currentPos.x][target.currentPos.y + rookPosY].currentPos);
            std::replace(posOfPieces.begin(), posOfPieces.end(), target.currentPos, start.currentPos);

            break;
        }
        
        case CAPTURE:{
            int colorOpp = (target.color == WHITE) ? BLACK : WHITE;

            //posOfPieces.erase(std::remove(posOfPieces.begin(), posOfPieces.end(), target.currentPos), posOfPieces.end());
            std::replace(posOfPieces.begin(), posOfPieces.end(), target.currentPos, start.currentPos);
            posOfPieces.push_back(target.currentPos);

            start.setPiece(target);
            target.setPiece(move.capturedPiece, colorOpp);

            break;
        }

        case EP_CAPTURE:{
            int direction = (start.color == WHITE) ? -1 : 1;
            int colorOpp = (target.color == WHITE) ? BLACK : WHITE;

            std::replace(posOfPieces.begin(), posOfPieces.end(), target.currentPos, start.currentPos);
            posOfPieces.push_back(tiles[target.currentPos.x - direction][target.currentPos.y].currentPos);

            start.setPiece(target);
            tiles[target.currentPos.x + direction][target.currentPos.y].setPiece(PAWN, colorOpp);
            target.setPiece(NONE, -1);

            enPassantActive = true;
            enPassantTarget = {target.currentPos.x, target.currentPos.y};

            break;

        }
        case N_PROMOTION:
        case B_PROMOTION:
        case R_PROMOTION:
        case Q_PROMOTION:
        case N_PROMOTION_CAPTURE:
        case B_PROMOTION_CAPTURE:
        case R_PROMOTION_CAPTURE:
        case Q_PROMOTION_CAPTURE:{
            break;
        }
    }

    

    changeTurn();
}

void Board::redoMove(){
    /*
    if(!redoStack.empty()){
        string redo = redoStack.top();
        undoStack.push(posToFen());

        parseFENPosition(redo);
        redoStack.pop();
    }*/
}


//  its takes as a parameter a tile that is already initialised with pseudo-legal moves
//  for every legal moves we make them and create another board where the move happened and check every opponent responses
//  if one of the opponent response is a king capture we delete the move from the tile from the parameter

vector<Move> Board::filterLegalMoves(Tile t){
    int opponent = (t.color == WHITE) ? BLACK : WHITE;
    vector<Move> validMoves;
    

    for(const Move& move : t.legalMoves){
        Board b = Board(*this);

        b.tryMakeMove(move);
        bool isKingSafe = true;

        for(int i = 0; i < BOARD; i++){
            for(int j = 0; j < BOARD; j++){
                Tile oppPiece = b.tiles[i][j];

                if(oppPiece.color == opponent){
                    b.generateMoves(oppPiece);

                    for(Move opp : oppPiece.legalMoves){
                        if(opp.type == CAPTURE && b.tiles[opp.target.x][opp.target.y].piece == KING){
                            isKingSafe = false;
                            break;
                        }
                    }
                }

                if (!isKingSafe) break;
            }
        }

        if(isKingSafe) validMoves.push_back(move);
    }

    return validMoves;
}




bool Board::isKingInCheck(unsigned int colorPiece){
    int colorOpp = (colorPiece == WHITE) ? BLACK : WHITE;

    for(Position opp : posOfPieces){
        Tile oppPiece = tiles[opp.x][opp.y];

        if(oppPiece.color == colorOpp && oppPiece.piece != KING){
            generateMoves(oppPiece);

            for(Move move : oppPiece.legalMoves){
                if(move.type == CAPTURE && tiles[move.target.x][move.target.y].piece == KING){
                    return true;
                }
            }
        }
    }

    return false;
}













void Board::displayBoard(){

    cout << "Board:\n";
    for(int i = 0; i < BOARD; i++){
        for(int j = 0; j < BOARD; j++){
            cout << tiles[i][j].piece;
            if(j != BOARD - 1){
                cout << ", ";
            }
        }
        cout << "\n";
    }

    cout << "isTheKingInCheck: " << isKingInCheck(WHITE) << endl;
}

void Board::displayPosTiles(){
    cout << "Board:\n";
    for(int i = 0; i < BOARD; i++){
        for(int j = 0; j < BOARD; j++){
            cout << posTiles[i][j];
            if(j != BOARD - 1){
                cout << ", ";
            }
        }
        cout << "\n";
    }
}



int Board::getAllMoves(Move move[], int size){
    int i = 0;

    while (i < size){
        for(int k = 0; k < BOARD; k++){
            for(int j = 0; j < BOARD; j++){
                if(tiles[k][j].piece != NONE){
                    generateLegalMoves(tiles[k][j]);

                    for(Move moves : tiles[k][j].legalMoves){
                        move[i] = moves;
                        i++;
                    }
                }
                
            }
        }
        return i;
    }
    return size;
}