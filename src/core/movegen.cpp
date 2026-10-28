#include "core/board.h"

namespace chess {

namespace {

// ---- 0x88 direction offsets ---------------------------------------------------
//
//  One rank up is +16 and one file right is +1, so every direction is a fixed number:
//
//        +15  +16  +17
//         -1    X   +1
//        -17  -16  -15

constexpr int North = 16, South = -16, East = 1, West = -1;

constexpr int KnightOffsets[] = {33, 31, 18, 14, -14, -18, -31, -33};
constexpr int KingOffsets[] = {North, South, East, West, 17, 15, -15, -17};
constexpr int BishopDirections[] = {17, 15, -15, -17};
constexpr int RookDirections[] = {North, South, East, West};

constexpr PieceType PromotionPieces[] = {Queen, Rook, Bishop, Knight};

constexpr int pawnPush(Color c) { return c == White ? North : South; }

}  // namespace


//  Instead of generating the attacker's moves, look outwards from the square: a knight
//  one knight jump away attacks it, a bishop or queen at the end of a free diagonal attacks it...

bool Board::isSquareAttacked(Square s, Color by) const {
    //  A white pawn attacks diagonally upwards, so it attacks s from one rank below
    int pawnFrom = by == White ? -16 : 16;
    for (int side : {-1, 1}) {
        Square from = s + pawnFrom + side;
        if (isOnBoard(from) && squares_[from] == makePiece(by, Pawn)) return true;
    }

    for (int offset : KnightOffsets) {
        Square from = s + offset;
        if (isOnBoard(from) && squares_[from] == makePiece(by, Knight)) return true;
    }

    for (int offset : KingOffsets) {
        Square from = s + offset;
        if (isOnBoard(from) && squares_[from] == makePiece(by, King)) return true;
    }

    //  Sliders: walk each ray until the first piece, it attacks s if it slides that way
    auto slidingAttack = [&](const int (&directions)[4], PieceType slider) {
        for (int d : directions) {
            for (Square from = s + d; isOnBoard(from); from = from + d) {
                Piece p = squares_[from];
                if (p == NoPiece) continue;
                if (p == makePiece(by, slider) || p == makePiece(by, Queen)) return true;
                break;
            }
        }
        return false;
    };

    return slidingAttack(BishopDirections, Bishop) || slidingAttack(RookDirections, Rook);
}


//  A move is legal when, after playing it, our king is not attacked. Playing it and checking
//  covers every special case at once: pins, moving into check, en passant discovered checks.

void Board::addIfLegal(MoveList& moves, Move m) {
    Color us = sideToMove_;
    makeMove(m);
    bool legal = !isSquareAttacked(kings_[us], ~us);
    unmakeMove();

    if (legal) moves.push(m);
}


void Board::generateLegalMoves(MoveList& moves) {
    moves.clear();
    Color us = sideToMove_;

    for (int rank = 0; rank < 8; rank++) {
        for (int file = 0; file < 8; file++) {
            Square from = makeSquare(file, rank);
            Piece piece = squares_[from];
            if (piece == NoPiece || colorOf(piece) != us) continue;

            //  A target is fine if it is empty or holds an enemy piece
            auto canLandOn = [&](Square to) {
                return squares_[to] == NoPiece || colorOf(squares_[to]) != us;
            };

            //  Knight and king: one step in each direction
            auto step = [&](const auto& offsets) {
                for (int offset : offsets) {
                    Square to = from + offset;
                    if (isOnBoard(to) && canLandOn(to)) addIfLegal(moves, Move(from, to));
                }
            };

            //  Bishop, rook and queen: keep going until the edge or a piece, captures stop the ray
            auto slide = [&](const auto& directions) {
                for (int d : directions) {
                    for (Square to = from + d; isOnBoard(to) && canLandOn(to); to = to + d) {
                        addIfLegal(moves, Move(from, to));
                        if (squares_[to] != NoPiece) break;
                    }
                }
            };

            switch (typeOf(piece)) {
                case Pawn: generatePawnMoves(moves, from); break;
                case Knight: step(KnightOffsets); break;
                case King: step(KingOffsets); break;
                case Bishop: slide(BishopDirections); break;
                case Rook: slide(RookDirections); break;
                case Queen: slide(BishopDirections); slide(RookDirections); break;
                default: break;
            }
        }
    }

    generateCastling(moves);
}


void Board::generatePawnMoves(MoveList& moves, Square from) {
    Color us = sideToMove_;
    int push = pawnPush(us);
    int startRank = us == White ? 1 : 6;
    int lastRank = us == White ? 7 : 0;

    //  Promotions come as four moves, one per piece
    auto addPawnMove = [&](Square to, MoveKind kind) {
        if (rankOf(to) == lastRank) {
            for (PieceType promotion : PromotionPieces) addIfLegal(moves, Move(from, to, kind, promotion));
        } else {
            addIfLegal(moves, Move(from, to, kind));
        }
    };

    Square oneUp = from + push;
    if (isOnBoard(oneUp) && squares_[oneUp] == NoPiece) {
        addPawnMove(oneUp, MoveKind::Normal);

        Square twoUp = oneUp + push;
        if (rankOf(from) == startRank && squares_[twoUp] == NoPiece) {
            addIfLegal(moves, Move(from, twoUp, MoveKind::DoublePush));
        }
    }

    for (int side : {-1, 1}) {
        Square to = from + push + side;
        if (!isOnBoard(to)) continue;

        if (to == enPassant_) {
            addIfLegal(moves, Move(from, to, MoveKind::EnPassant));
        } else if (squares_[to] != NoPiece && colorOf(squares_[to]) != us) {
            addPawnMove(to, MoveKind::Normal);
        }
    }
}


//  Castling needs the right (king and rook never moved, see setFen and makeMove), the squares
//  between them empty, and the king not in check and not passing over an attacked square.
//  Landing on an attacked square is caught by addIfLegal like any other move.

void Board::generateCastling(MoveList& moves) {
    Color us = sideToMove_;
    Color them = ~us;

    bool white = us == White;
    Square king = white ? E1 : E8;
    int kingside = white ? WhiteKingside : BlackKingside;
    int queenside = white ? WhiteQueenside : BlackQueenside;

    if (!(castlingRights_ & (kingside | queenside))) return;
    if (isSquareAttacked(king, them)) return;

    auto empty = [&](Square s) { return squares_[s] == NoPiece; };

    if ((castlingRights_ & kingside) && empty(king + 1) && empty(king + 2) && !isSquareAttacked(king + 1, them)) {
        addIfLegal(moves, Move(king, king + 2, MoveKind::Castling));
    }
    if ((castlingRights_ & queenside) && empty(king - 1) && empty(king - 2) && empty(king - 3) && !isSquareAttacked(king - 1, them)) {
        addIfLegal(moves, Move(king, king - 2, MoveKind::Castling));
    }
}


Move Board::moveFromUci(std::string_view uci) {
    MoveList moves;
    generateLegalMoves(moves);

    for (Move m : moves) {
        if (m.toUci() == uci) return m;
    }
    return Move();
}


GameStatus Board::status() {
    MoveList moves;
    generateLegalMoves(moves);

    if (moves.empty()) return inCheck() ? GameStatus::Checkmate : GameStatus::Stalemate;
    if (halfmoveClock_ >= 100) return GameStatus::FiftyMoveRule;
    if (repetitions() >= 2) return GameStatus::ThreefoldRepetition;
    if (hasInsufficientMaterial()) return GameStatus::InsufficientMaterial;
    return GameStatus::Ongoing;
}

}  // namespace chess
