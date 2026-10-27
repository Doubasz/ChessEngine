#include "search/eval.h"

#include <array>

namespace chess {

namespace {

constexpr std::array<int, 7> PieceValues = {0, 100, 320, 330, 500, 900, 0};

// ---- Piece-square tables -------------------------------------------------------
//
//  From Tomasz Michniewski's "Simplified Evaluation Function". Written as white sees the board:
//  the first row is rank 8, the last row is rank 1. Black reads them upside down.

using Table = std::array<int, 64>;

constexpr Table PawnTable = {
     0,   0,   0,   0,   0,   0,   0,   0,
    50,  50,  50,  50,  50,  50,  50,  50,
    10,  10,  20,  30,  30,  20,  10,  10,
     5,   5,  10,  25,  25,  10,   5,   5,
     0,   0,   0,  20,  20,   0,   0,   0,
     5,  -5, -10,   0,   0, -10,  -5,   5,
     5,  10,  10, -20, -20,  10,  10,   5,
     0,   0,   0,   0,   0,   0,   0,   0,
};

constexpr Table KnightTable = {
   -50, -40, -30, -30, -30, -30, -40, -50,
   -40, -20,   0,   0,   0,   0, -20, -40,
   -30,   0,  10,  15,  15,  10,   0, -30,
   -30,   5,  15,  20,  20,  15,   5, -30,
   -30,   0,  15,  20,  20,  15,   0, -30,
   -30,   5,  10,  15,  15,  10,   5, -30,
   -40, -20,   0,   5,   5,   0, -20, -40,
   -50, -40, -30, -30, -30, -30, -40, -50,
};

constexpr Table BishopTable = {
   -20, -10, -10, -10, -10, -10, -10, -20,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -10,   0,   5,  10,  10,   5,   0, -10,
   -10,   5,   5,  10,  10,   5,   5, -10,
   -10,   0,  10,  10,  10,  10,   0, -10,
   -10,  10,  10,  10,  10,  10,  10, -10,
   -10,   5,   0,   0,   0,   0,   5, -10,
   -20, -10, -10, -10, -10, -10, -10, -20,
};

constexpr Table RookTable = {
     0,   0,   0,   0,   0,   0,   0,   0,
     5,  10,  10,  10,  10,  10,  10,   5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
    -5,   0,   0,   0,   0,   0,   0,  -5,
     0,   0,   0,   5,   5,   0,   0,   0,
};

constexpr Table QueenTable = {
   -20, -10, -10,  -5,  -5, -10, -10, -20,
   -10,   0,   0,   0,   0,   0,   0, -10,
   -10,   0,   5,   5,   5,   5,   0, -10,
    -5,   0,   5,   5,   5,   5,   0,  -5,
     0,   0,   5,   5,   5,   5,   0,  -5,
   -10,   5,   5,   5,   5,   5,   0, -10,
   -10,   0,   5,   0,   0,   0,   0, -10,
   -20, -10, -10,  -5,  -5, -10, -10, -20,
};

//  Middlegame: stay castled behind the pawns
constexpr Table KingTable = {
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -30, -40, -40, -50, -50, -40, -40, -30,
   -20, -30, -30, -40, -40, -30, -30, -20,
   -10, -20, -20, -20, -20, -20, -20, -10,
    20,  20,   0,   0,   0,   0,  20,  20,
    20,  30,  10,   0,   0,  10,  30,  20,
};

//  Endgame: the king becomes a fighting piece and heads for the center
constexpr Table KingEndgameTable = {
   -50, -40, -30, -20, -20, -30, -40, -50,
   -30, -20, -10,   0,   0, -10, -20, -30,
   -30, -10,  20,  30,  30,  20, -10, -30,
   -30, -10,  30,  40,  40,  30, -10, -30,
   -30, -10,  30,  40,  40,  30, -10, -30,
   -30, -10,  20,  30,  30,  20, -10, -30,
   -30, -30,   0,   0,   0,   0, -30, -30,
   -50, -30, -30, -30, -30, -30, -30, -50,
};

constexpr std::array<const Table*, 7> Tables = {
    nullptr, &PawnTable, &KnightTable, &BishopTable, &RookTable, &QueenTable, &KingTable,
};

//  Index in a table for a piece of color c on s: white reads rank 8 first, black is mirrored
int tableIndex(Color c, Square s) {
    int rank = c == White ? 7 - rankOf(s) : rankOf(s);
    return rank * 8 + fileOf(s);
}

}  // namespace


int pieceValue(PieceType type) {
    return PieceValues[type];
}

int evaluate(const Board& board) {
    std::array<int, 2> score{};

    //  For the endgame test: the queens, and the rooks and minor pieces next to them
    std::array<bool, 2> hasQueen{};
    std::array<int, 2> otherMaterial{};

    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            Square s = makeSquare(file, rank);
            Piece piece = board.pieceAt(s);
            if (piece == NoPiece || typeOf(piece) == King) continue;

            Color c = colorOf(piece);
            PieceType type = typeOf(piece);
            score[c] += PieceValues[type] + (*Tables[type])[tableIndex(c, s)];

            if (type == Queen) hasQueen[c] = true;
            else if (type != Pawn) otherMaterial[c] += PieceValues[type];
        }
    }

    //  Endgame once each side has no queen, or a queen and at most one minor piece
    bool endgame = true;
    for (Color c : {White, Black}) {
        if (hasQueen[c] && otherMaterial[c] > PieceValues[Bishop]) endgame = false;
    }

    const Table& kingTable = endgame ? KingEndgameTable : KingTable;
    for (Color c : {White, Black}) {
        score[c] += kingTable[tableIndex(c, board.kingSquare(c))];
    }

    int whiteScore = score[White] - score[Black];
    return board.sideToMove() == White ? whiteScore : -whiteScore;
}

}  // namespace chess
