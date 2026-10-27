#include "core/board.h"

#include <charconv>
#include <sstream>

namespace chess {

namespace {

bool parseCounter(const std::string& field, int& out) {
    const char* end = field.data() + field.size();
    auto [ptr, error] = std::from_chars(field.data(), end, out);
    return error == std::errc() && ptr == end && out >= 0;
}

}  // namespace


Board::Board(EmptyTag) {}

Board::Board() {
    setFen(StartFen);
}


//  FEN fields, separated by spaces:
//      1. pieces, rank 8 first, each rank from a to h. Digits are runs of empty squares
//      2. side to move, w or b
//      3. castling rights, any of KQkq, or -
//      4. en passant target square (the square the pawn skipped), or -
//      5. halfmove clock, 6. fullmove number (both optional here)

bool Board::setFen(std::string_view fen) {
    std::istringstream fields{std::string(fen)};
    std::string placement, side, castling, enPassant;
    std::string halfmove = "0", fullmove = "1";

    if (!(fields >> placement >> side >> castling >> enPassant)) return false;
    fields >> halfmove >> fullmove;

    //  Parsed into a separate board so a bad FEN leaves this one untouched
    Board parsed{EmptyTag{}};

    // 1. Pieces
    int rank = 7, file = 0;
    for (char c : placement) {
        if (c == '/') {
            if (file != 8 || rank == 0) return false;
            rank--;
            file = 0;
        } else if (c >= '1' && c <= '8') {
            file += c - '0';
            if (file > 8) return false;
        } else {
            Piece piece = pieceFromChar(c);
            if (piece == NoPiece || file > 7) return false;

            Square s = makeSquare(file, rank);
            parsed.squares_[s] = piece;

            if (typeOf(piece) == King) {
                if (parsed.kings_[colorOf(piece)] != NoSquare) return false;  // two kings of one color
                parsed.kings_[colorOf(piece)] = s;
            }
            file++;
        }
    }
    if (rank != 0 || file != 8) return false;
    if (parsed.kings_[White] == NoSquare || parsed.kings_[Black] == NoSquare) return false;

    // 2. Side to move
    if (side == "w") parsed.sideToMove_ = White;
    else if (side == "b") parsed.sideToMove_ = Black;
    else return false;

    // 3. Castling rights. A right only counts if the king and rook are still on their
    //    starting squares, so the move generator can trust it without checking the pieces
    if (castling != "-") {
        for (char c : castling) {
            switch (c) {
                case 'K': parsed.castlingRights_ |= WhiteKingside; break;
                case 'Q': parsed.castlingRights_ |= WhiteQueenside; break;
                case 'k': parsed.castlingRights_ |= BlackKingside; break;
                case 'q': parsed.castlingRights_ |= BlackQueenside; break;
                default: return false;
            }
        }
    }
    const auto& sq = parsed.squares_;
    if (sq[E1] != WhiteKing || sq[H1] != WhiteRook) parsed.castlingRights_ &= ~WhiteKingside;
    if (sq[E1] != WhiteKing || sq[A1] != WhiteRook) parsed.castlingRights_ &= ~WhiteQueenside;
    if (sq[E8] != BlackKing || sq[H8] != BlackRook) parsed.castlingRights_ &= ~BlackKingside;
    if (sq[E8] != BlackKing || sq[A8] != BlackRook) parsed.castlingRights_ &= ~BlackQueenside;

    // 4. En passant: the skipped square is on rank 6 when white is to move, rank 3 when black is
    if (enPassant != "-") {
        Square s = squareFromString(enPassant);
        int expectedRank = parsed.sideToMove_ == White ? 5 : 2;
        if (s == NoSquare || rankOf(s) != expectedRank) return false;
        parsed.enPassant_ = s;
    }

    // 5 and 6. Counters
    if (!parseCounter(halfmove, parsed.halfmoveClock_)) return false;
    if (!parseCounter(fullmove, parsed.fullmoveNumber_) || parsed.fullmoveNumber_ == 0) return false;

    *this = parsed;
    return true;
}


std::string Board::fen() const {
    std::string fen;

    for (int rank = 7; rank >= 0; rank--) {
        int empty = 0;
        for (int file = 0; file < 8; file++) {
            Piece piece = squares_[makeSquare(file, rank)];
            if (piece == NoPiece) {
                empty++;
                continue;
            }
            if (empty > 0) fen += char('0' + empty);
            empty = 0;
            fen += pieceToChar(piece);
        }
        if (empty > 0) fen += char('0' + empty);
        if (rank > 0) fen += '/';
    }

    fen += sideToMove_ == White ? " w " : " b ";

    if (castlingRights_ == NoCastling) fen += '-';
    if (castlingRights_ & WhiteKingside) fen += 'K';
    if (castlingRights_ & WhiteQueenside) fen += 'Q';
    if (castlingRights_ & BlackKingside) fen += 'k';
    if (castlingRights_ & BlackQueenside) fen += 'q';

    fen += ' ' + squareToString(enPassant_);
    fen += ' ' + std::to_string(halfmoveClock_);
    fen += ' ' + std::to_string(fullmoveNumber_);

    return fen;
}

}  // namespace chess
