#include "search/eval.h"

#include <array>
#include <cstdint>

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


// ---- Pawn structure -------------------------------------------------------------

constexpr int DoubledPawnPenalty = 15;  // for each pawn more than one on a file
constexpr int IsolatedPawnPenalty = 15;

//  Indexed by how far the passed pawn has advanced, 1 on its starting rank, 6 one step from promotion
constexpr std::array<int, 8> PassedPawnBonus = {0, 10, 15, 25, 40, 65, 100, 0};

//  Where the pawns of each color are: bit r of pawns[c][f] is set if c has a pawn on file f, rank r.
//  Filled by the same pass over the board that counts the material.
struct PawnMap {
    std::array<std::array<uint8_t, 8>, 2> pawns{};

    void add(Color c, Square s) { pawns[c][fileOf(s)] |= uint8_t(1 << rankOf(s)); }

    //  The pawns of c on file f, 0 if f is off the board
    uint8_t onFile(Color c, int f) const { return f >= 0 && f < 8 ? pawns[c][f] : 0; }
};

int countBits(uint8_t bits) {
    int count = 0;
    for (; bits != 0; bits &= uint8_t(bits - 1)) count++;
    return count;
}

//  The ranks in front of a pawn of color c on rank r, as a mask of rank bits
uint8_t ranksInFront(Color c, int r) {
    return c == White ? uint8_t(0xFF << (r + 1)) : uint8_t((1 << r) - 1);
}

int pawnScore(const PawnMap& map, Color c) {
    int score = 0;

    for (int f = 0; f < 8; f++) {
        uint8_t own = map.onFile(c, f);
        if (own == 0) continue;

        int count = countBits(own);
        score -= DoubledPawnPenalty * (count - 1);

        bool isolated = map.onFile(c, f - 1) == 0 && map.onFile(c, f + 1) == 0;
        if (isolated) score -= IsolatedPawnPenalty * count;

        uint8_t enemies = map.onFile(~c, f - 1) | map.onFile(~c, f) | map.onFile(~c, f + 1);
        for (int r = 0; r < 8; r++) {
            if ((own & (1 << r)) == 0) continue;
            if ((enemies & ranksInFront(c, r)) != 0) continue;

            int advance = c == White ? r : 7 - r;
            score += PassedPawnBonus[advance];
        }
    }
    return score;
}

PawnMap pawnMapOf(const Board& board) {
    PawnMap map;
    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            Square s = makeSquare(file, rank);
            Piece piece = board.pieceAt(s);
            if (typeOf(piece) == Pawn) map.add(colorOf(piece), s);
        }
    }
    return map;
}

}  // namespace


int pieceValue(PieceType type) {
    return PieceValues[type];
}

int pawnStructure(const Board& board, Color c) {
    return pawnScore(pawnMapOf(board), c);
}

int evaluate(const Board& board) {
    std::array<int, 2> score{};

    //  For the endgame test: the queens, and the rooks and minor pieces next to them
    std::array<bool, 2> hasQueen{};
    std::array<int, 2> otherMaterial{};
    PawnMap pawns;

    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            Square s = makeSquare(file, rank);
            Piece piece = board.pieceAt(s);
            if (piece == NoPiece || typeOf(piece) == King) continue;

            Color c = colorOf(piece);
            PieceType type = typeOf(piece);
            score[c] += PieceValues[type] + (*Tables[type])[tableIndex(c, s)];

            if (type == Pawn) pawns.add(c, s);
            else if (type == Queen) hasQueen[c] = true;
            else otherMaterial[c] += PieceValues[type];
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
        score[c] += pawnScore(pawns, c);
    }

    int whiteScore = score[White] - score[Black];
    return board.sideToMove() == White ? whiteScore : -whiteScore;
}

}  // namespace chess
