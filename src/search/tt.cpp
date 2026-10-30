#include "search/tt.h"
#include "search/search.h"

#include <algorithm>

namespace chess {

static_assert(sizeof(TableEntry) == 16, "keep entries small, the table holds millions of them");

TranspositionTable::TranspositionTable(size_t sizeMb) {
    //  The largest power of 2 of entries that fits in the size
    size_t count = 1;
    while (count * 2 * sizeof(TableEntry) <= sizeMb * 1024 * 1024) count *= 2;

    entries_.resize(count);
    mask_ = count - 1;
}

const TableEntry* TranspositionTable::probe(uint64_t key) const {
    const TableEntry& entry = entries_[key & mask_];
    return entry.bound != Bound::None && entry.key == key ? &entry : nullptr;
}

void TranspositionTable::store(uint64_t key, int depth, int score, Bound bound, Move move) {
    TableEntry& entry = entries_[key & mask_];
    if (entry.key == key && entry.depth > depth) return;

    //  A search that failed low has no best move, keep the one an earlier search found
    if (move.isNull() && entry.key == key) move = entry.move;

    entry.key = key;
    entry.move = move;
    entry.score = int16_t(score);
    entry.depth = uint8_t(depth);
    entry.bound = bound;
}

void TranspositionTable::clear() {
    std::fill(entries_.begin(), entries_.end(), TableEntry());
}

int scoreToTable(int score, int ply) {
    if (score >= MateScore - MaxPly) return score + ply;
    if (score <= -(MateScore - MaxPly)) return score - ply;
    return score;
}

int scoreFromTable(int score, int ply) {
    if (score >= MateScore - MaxPly) return score - ply;
    if (score <= -(MateScore - MaxPly)) return score + ply;
    return score;
}

}  // namespace chess
