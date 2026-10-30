#pragma once

#include "core/move.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace chess {

//  What a search of a position found, kept so the position is not searched again when another
//  move order reaches it, or at the next iteration or the next move.
//
//  The search is alpha-beta, so the score it returns is often only a bound: a move reached beta
//  and the other moves were skipped (the score is at least that, Lower), or no move reached alpha
//  (at most that, Upper).
enum class Bound : uint8_t { None, Exact, Lower, Upper };

struct TableEntry {
    uint64_t key = 0;  // the full hash: entries are found by its low bits, so different positions share a slot
    Move move;  // the best move found, null if none (every move failed low)
    int16_t score = 0;  // mate scores are counted from this position, see scoreToTable
    uint8_t depth = 0;
    Bound bound = Bound::None;
};

//  A fixed size table of entries, indexed by the Zobrist hash of the position. When two positions
//  fall on the same slot, the newer one replaces the older, unless the older is the same
//  position searched deeper.
class TranspositionTable {
public:
    static constexpr size_t DefaultSizeMb = 16;

    explicit TranspositionTable(size_t sizeMb = DefaultSizeMb);

    //  The entry stored for this position, nullptr if there is none
    const TableEntry* probe(uint64_t key) const;
    void store(uint64_t key, int depth, int score, Bound bound, Move move);

    void clear();
    size_t capacity() const { return entries_.size(); }

private:
    std::vector<TableEntry> entries_;  // a power of 2 of them, so the slot is key & mask_
    uint64_t mask_ = 0;
};

//  A mate score in the search is counted from the root: mated in 3 plies from the root.
//  In the table it must be counted from the stored position, which the search can reach at any
//  ply: there it is mated in 3 - ply plies. ply is the distance from the root to the position.
int scoreToTable(int score, int ply);
int scoreFromTable(int score, int ply);

}  // namespace chess
