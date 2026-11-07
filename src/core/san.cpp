#include "core/san.h"

namespace chess {

namespace {

//  SAN piece letters are uppercase for both colors
char pieceLetter(PieceType type) {
    return pieceToChar(makePiece(White, type));
}

//  What tells m apart from the other legal moves of a piece of the same type to the same square:
//  nothing if there is no such move, else the file, else the rank, else the whole square

std::string disambiguation(Board& board, Move m, PieceType type) {
    MoveList moves;
    board.generateLegalMoves(moves);

    bool ambiguous = false, sameFile = false, sameRank = false;
    for (Move other : moves) {
        if (other.to() != m.to() || other.from() == m.from()) continue;
        if (typeOf(board.pieceAt(other.from())) != type) continue;

        ambiguous = true;
        if (fileOf(other.from()) == fileOf(m.from())) sameFile = true;
        if (rankOf(other.from()) == rankOf(m.from())) sameRank = true;
    }

    if (!ambiguous) return "";
    if (!sameFile) return std::string(1, char('a' + fileOf(m.from())));
    if (!sameRank) return std::string(1, char('1' + rankOf(m.from())));
    return squareToString(m.from());
}

}  // namespace


std::string toSan(const Board& position, Move m) {
    Board board = position;  // generateLegalMoves and makeMove need a board they can change

    PieceType type = typeOf(board.pieceAt(m.from()));
    bool capture = board.pieceAt(m.to()) != NoPiece || m.kind() == MoveKind::EnPassant;

    std::string san;
    if (m.kind() == MoveKind::Castling) {
        //  The king moves to the g file on the kingside, to the c file on the queenside
        san = fileOf(m.to()) > fileOf(m.from()) ? "O-O" : "O-O-O";
    } else if (type == Pawn) {
        if (capture) {
            san += char('a' + fileOf(m.from()));
            san += 'x';
        }
        san += squareToString(m.to());
        if (m.promotion() != NoPieceType) {
            san += '=';
            san += pieceLetter(m.promotion());
        }
    } else {
        san += pieceLetter(type);
        san += disambiguation(board, m, type);
        if (capture) san += 'x';
        san += squareToString(m.to());
    }

    board.makeMove(m);
    if (board.inCheck()) {
        MoveList replies;
        board.generateLegalMoves(replies);
        san += replies.empty() ? '#' : '+';
    }
    return san;
}

}  // namespace chess
