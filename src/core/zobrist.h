#pragma once

#include "core/types.h"

#include <array>
#include <cstdint>

//  Zobrist hashing: a random 64-bit key for every feature a position can have (a given piece on
//  a given square, black to move, a set of castling rights, an en passant file). The hash of a
//  position is the XOR of the keys of its features. XOR is its own inverse, so a move updates the
//  hash by XORing in the keys of the features it removes and of the ones it adds.
//
//  The keys come from a generator with a fixed seed, built at compile time: the same position
//  has the same hash on every run and every machine.

namespace chess::zobrist {

//  splitmix64, a small generator with well mixed output, see https://prng.di.unimi.it/splitmix64.c
constexpr uint64_t nextRandom(uint64_t& state) {
    state += 0x9E3779B97F4A7C15ull;
    uint64_t z = state;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

struct Keys {
    //  Indexed by Piece and by 0x88 square: the unused entries (no piece, off board) cost a few
    //  kilobytes and save converting the square on every lookup
    std::array<std::array<uint64_t, BoardArraySize>, 16> pieceSquare{};
    std::array<uint64_t, 16> castling{};  // indexed by the 4 bits of the castling rights
    std::array<uint64_t, 8> enPassantFile{};
    uint64_t blackToMove = 0;
};

constexpr Keys makeKeys() {
    Keys keys;
    uint64_t state = 0x2545F4914F6CDD1Dull;

    for (auto& squares : keys.pieceSquare)
        for (auto& key : squares) key = nextRandom(state);

    //  No castling rights is the most common case, its key is 0 so it costs nothing
    for (size_t rights = 1; rights < keys.castling.size(); rights++) keys.castling[rights] = nextRandom(state);

    for (auto& key : keys.enPassantFile) key = nextRandom(state);
    keys.blackToMove = nextRandom(state);
    return keys;
}

inline constexpr Keys keys = makeKeys();

}  // namespace chess::zobrist
