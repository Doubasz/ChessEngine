#pragma once

#include "core/types.h"

#include <array>
#include <cstddef>
#include <string>

namespace chess {

//  What makeMove has to do besides moving one piece. Captures are not a separate kind:
//  makeMove reads the captured piece from the board, so a Normal move can be a capture.
enum class MoveKind : uint8_t {
    Normal,
    DoublePush,  // pawn two squares forward, sets the en passant square
    EnPassant,  // the captured pawn is not on the target square
    Castling,  // the king's move, makeMove also moves the rook
};

//  4 bytes, cheap to copy and to store by the hundred in a MoveList
class Move {
public:
    constexpr Move() = default;  // the null move, see isNull()
    constexpr Move(Square from, Square to, MoveKind kind = MoveKind::Normal, PieceType promotion = NoPieceType)
        : from_(uint8_t(from)), to_(uint8_t(to)), kind_(kind), promotion_(promotion) {}

    constexpr Square from() const { return Square(from_); }
    constexpr Square to() const { return Square(to_); }
    constexpr MoveKind kind() const { return kind_; }
    constexpr PieceType promotion() const { return promotion_; }  // NoPieceType if not a promotion
    constexpr bool isNull() const { return from_ == to_; }

    //  UCI notation: e2e4, e1g1 for castling, e7e8q for promotions, 0000 for the null move
    std::string toUci() const;

    constexpr bool operator==(const Move& other) const {
        return from_ == other.from_ && to_ == other.to_ && kind_ == other.kind_ && promotion_ == other.promotion_;
    }
    constexpr bool operator!=(const Move& other) const { return !(*this == other); }

private:
    uint8_t from_ = 0;
    uint8_t to_ = 0;
    MoveKind kind_ = MoveKind::Normal;
    PieceType promotion_ = NoPieceType;
};


//  Fixed size list filled by the move generator, no heap allocation.
//  Only legal moves are stored and a chess position has at most 218 of them.
class MoveList {
public:
    static constexpr size_t Capacity = 256;

    void push(Move m) { moves_[size_++] = m; }
    void clear() { size_ = 0; }

    size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    Move operator[](size_t i) const { return moves_[i]; }

    const Move* begin() const { return moves_.data(); }
    const Move* end() const { return moves_.data() + size_; }

private:
    std::array<Move, Capacity> moves_;
    size_t size_ = 0;
};

}  // namespace chess
