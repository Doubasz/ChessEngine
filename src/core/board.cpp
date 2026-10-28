#include "core/board.h"
#include "core/zobrist.h"

#include <algorithm>
#include <charconv>
#include <initializer_list>
#include <sstream>

namespace chess {

namespace {

bool parseCounter(const std::string& field, int& out) {
    const char* end = field.data() + field.size();
    auto [ptr, error] = std::from_chars(field.data(), end, out);
    return error == std::errc() && ptr == end && out >= 0;
}

//  Castling rights that survive a move touching each square: anything leaving or landing on
//  e1 loses both white rights, a1 loses white queenside... 15 (all kept) everywhere else.
//  makeMove does rights &= mask[from] & mask[to], which also handles a rook being captured.
constexpr std::array<uint8_t, BoardArraySize> CastlingMask = [] {
    std::array<uint8_t, BoardArraySize> mask{};
    for (auto& m : mask) m = AllCastling;
    mask[E1] = AllCastling & ~(WhiteKingside | WhiteQueenside);
    mask[H1] = AllCastling & ~WhiteKingside;
    mask[A1] = AllCastling & ~WhiteQueenside;
    mask[E8] = AllCastling & ~(BlackKingside | BlackQueenside);
    mask[H8] = AllCastling & ~BlackKingside;
    mask[A8] = AllCastling & ~BlackQueenside;
    return mask;
}();

//  Where the rook starts and ends for a castling move, keyed by the king's target square
struct RookMove {
    Square from, to;
};

constexpr RookMove castlingRook(Square kingTo) {
    switch (kingTo) {
        case G1: return {H1, F1};
        case C1: return {A1, D1};
        case G8: return {H8, F8};
        default: return {A8, D8};  // C8
    }
}

//  The pawn taken en passant is beside the moving pawn, one rank behind the target square
constexpr Square enPassantVictim(Square to, Color us) {
    return us == White ? to - 16 : to + 16;
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

    // 4. En passant: the skipped square is on rank 6 when white is to move, rank 3 when black is.
    //    It is kept only if it is empty with the pawn that just moved in front of it, otherwise
    //    an en passant capture would remove whatever piece stands there
    if (enPassant != "-") {
        Square s = squareFromString(enPassant);
        int expectedRank = parsed.sideToMove_ == White ? 5 : 2;
        if (s == NoSquare || rankOf(s) != expectedRank) return false;

        Square pushedPawn = parsed.sideToMove_ == White ? s - 16 : s + 16;
        if (sq[s] == NoPiece && sq[pushedPawn] == makePiece(~parsed.sideToMove_, Pawn)) parsed.enPassant_ = s;
    }

    // 5 and 6. Counters
    if (!parseCounter(halfmove, parsed.halfmoveClock_)) return false;
    if (!parseCounter(fullmove, parsed.fullmoveNumber_) || parsed.fullmoveNumber_ == 0) return false;

    parsed.hash_ = parsed.computeHash();
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


void Board::makeMove(Move m) {
    Color us = sideToMove_;
    Square from = m.from(), to = m.to();
    Piece piece = squares_[from];
    Piece placed = m.promotion() != NoPieceType ? makePiece(us, m.promotion()) : piece;

    Square captureSquare = m.kind() == MoveKind::EnPassant ? enPassantVictim(to, us) : to;
    Piece captured = squares_[captureSquare];

    history_.push_back({m, captured, castlingRights_, enPassant_, halfmoveClock_, hash_});

    //  Every change to the position XORs its key into the hash. The old castling rights and
    //  en passant file are taken out here, the new ones are put in at the end.
    const zobrist::Keys& keys = zobrist::keys;
    hash_ ^= keys.castling[castlingRights_] ^ enPassantKey();

    squares_[captureSquare] = NoPiece;
    squares_[from] = NoPiece;
    squares_[to] = placed;

    if (captured != NoPiece) hash_ ^= keys.pieceSquare[captured][captureSquare];
    hash_ ^= keys.pieceSquare[piece][from] ^ keys.pieceSquare[placed][to];

    if (m.kind() == MoveKind::Castling) {
        RookMove rook = castlingRook(to);
        Piece rookPiece = squares_[rook.from];
        squares_[rook.to] = rookPiece;
        squares_[rook.from] = NoPiece;
        hash_ ^= keys.pieceSquare[rookPiece][rook.from] ^ keys.pieceSquare[rookPiece][rook.to];
    }

    if (typeOf(piece) == King) kings_[us] = to;

    castlingRights_ &= CastlingMask[from] & CastlingMask[to];

    //  The en passant square is the one the pawn skipped
    enPassant_ = m.kind() == MoveKind::DoublePush ? from + (us == White ? 16 : -16) : NoSquare;

    halfmoveClock_ = (typeOf(piece) == Pawn || captured != NoPiece) ? 0 : halfmoveClock_ + 1;
    if (us == Black) fullmoveNumber_++;

    sideToMove_ = ~us;
    hash_ ^= keys.blackToMove ^ keys.castling[castlingRights_] ^ enPassantKey();
}


//  The exact reverse of makeMove, in the opposite order

void Board::unmakeMove() {
    Undo undo = history_.back();
    history_.pop_back();

    Move m = undo.move;
    Square from = m.from(), to = m.to();
    Color us = ~sideToMove_;  // the side that made the move
    sideToMove_ = us;

    if (us == Black) fullmoveNumber_--;
    halfmoveClock_ = undo.halfmoveClock;
    enPassant_ = undo.enPassant;
    castlingRights_ = undo.castlingRights;
    hash_ = undo.hash;

    //  A promoted piece goes back to being a pawn
    Piece piece = m.promotion() != NoPieceType ? makePiece(us, Pawn) : squares_[to];

    if (typeOf(piece) == King) kings_[us] = from;

    if (m.kind() == MoveKind::Castling) {
        RookMove rook = castlingRook(to);
        squares_[rook.from] = squares_[rook.to];
        squares_[rook.to] = NoPiece;
    }

    squares_[to] = NoPiece;
    squares_[from] = piece;

    Square captureSquare = m.kind() == MoveKind::EnPassant ? enPassantVictim(to, us) : to;
    squares_[captureSquare] = undo.captured;
}


// ---- Hashing ------------------------------------------------------------------

uint64_t Board::computeHash() const {
    const zobrist::Keys& keys = zobrist::keys;

    uint64_t hash = keys.castling[castlingRights_] ^ enPassantKey();
    if (sideToMove_ == Black) hash ^= keys.blackToMove;

    //  The padding squares of the 0x88 board are always empty
    for (int s = 0; s < BoardArraySize; s++) {
        if (squares_[s] != NoPiece) hash ^= keys.pieceSquare[squares_[s]][s];
    }
    return hash;
}

//  The en passant file counts in the hash only when a pawn of the side to move stands next to
//  the pawn that just moved, ready to take it (a pinned pawn still counts, as in most engines).
//  Otherwise every double push would make the position look different from the same position
//  reached by other moves, and repetitions would be missed.

uint64_t Board::enPassantKey() const {
    if (enPassant_ == NoSquare) return 0;

    Square pushedPawn = sideToMove_ == White ? enPassant_ - 16 : enPassant_ + 16;
    Piece ourPawn = makePiece(sideToMove_, Pawn);
    for (Square s : {pushedPawn - 1, pushedPawn + 1}) {
        if (isOnBoard(s) && squares_[s] == ourPawn) return zobrist::keys.enPassantFile[fileOf(enPassant_)];
    }
    return 0;
}


// ---- Draws --------------------------------------------------------------------

//  Only the positions since the last capture or pawn move can repeat, those moves cannot be
//  undone. And only every other one has the same side to move.

int Board::repetitions() const {
    int played = int(history_.size());
    int lookback = std::min(halfmoveClock_, played);

    int count = 0;
    for (int i = 2; i <= lookback; i += 2) {
        if (history_[played - i].hash == hash_) count++;
    }
    return count;
}

bool Board::hasInsufficientMaterial() const {
    int knights = 0, bishops = 0;
    bool bishopOnLight = false, bishopOnDark = false;

    for (int i = 0; i < BoardArraySize; i++) {
        Square s = Square(i);
        switch (typeOf(squares_[s])) {
            case NoPieceType:
            case King:
                break;
            case Knight:
                knights++;
                break;
            case Bishop:
                bishops++;
                ((fileOf(s) + rankOf(s)) % 2 == 0 ? bishopOnDark : bishopOnLight) = true;  // a1 is dark
                break;
            default:
                return false;  // a pawn, rook or queen can always mate
        }
    }

    if (knights + bishops <= 1) return true;
    return knights == 0 && !(bishopOnLight && bishopOnDark);
}

}  // namespace chess
