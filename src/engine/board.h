#ifndef _BOARD
#define _BOARD

#include "tile.h"
#include "position.h"
#include "GameState.h"
#include "move.h"
#include <vector>
#include <stack>


enum Castle{
    NO_CASTLE,
    W_CASTLE_KS,
    W_CASTLE_QS,
    B_CASTLE_KS,
    B_CASTLE_QS,
};

enum GameStatus{
    ONGOING,
    CHECKMATE,
    STALEMATE,
    DRAW_FIFTY_MOVES,
};

const int WHITE_KINGSIDE = 1; // 0001
const int WHITE_QUEENSIDE = 2; // 0010
const int BLACK_KINGSIDE = 4; // 0100
const int BLACK_QUEENSIDE = 8; // 1000


class Board{


    public:
        Tile tiles[8][8];
        std::vector<Position> posOfPieces;
        Move lastMove;

        std::stack<std::string> undoStack;
        std::stack<std::string> redoStack;

        GameState currentGameState;
        std::stack<GameState> gameStateHistory;

        int castlingRights;
        int prevCastlingRights;
        int turnToMove;
        bool enPassantActive;
        Position enPassantTarget;
        Position lastEnPassant;

        unsigned int nbMoves;
        unsigned int nbHalfMoves;



        Board();
        Board(const Board& other);
        void parseFENPosition(std::string pos);
        std::string posToFen();
        ~Board();

        void changeTurn();
        void generateMoves(Tile& tile);
        void generateAllMoves();
        void generateLegalMoves(Tile& tile);
        void tryMakeMove(Move move);
        void makePromotion(Tile& target, int choice);
        void makeMove(Move move);
        void undoLastMove();
        void undoMove(Move move);
        void tryUndoMove(Move move);
        void redoMove();
        void displayBoard();
        int getAllMoves(Move move[], int size);
        GameStatus getGameStatus();

        std::vector<Move> filterLegalMoves(Tile tile);
        bool isKingInCheck(unsigned int color);
        void updateCastlingRights();
        void restoreCastlingRights();

        void restoreGameState();
};

std::vector<std::string> split(const std::string& str, char delimiter);
bool notOutOfBonds(Position p);
Position getTileTillOutBonds(Position p);
std::vector<Position> getDirectionSlidingPiece(Tile t);
char pieceToFen(Tile t);
Position translateAlgebraic(std::string s);
bool isInValidMoves(std::vector<Move> v, Move move);

bool moveIsAnyPromCapt(Move move);



#endif