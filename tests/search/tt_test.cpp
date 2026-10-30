#include "search/search.h"
#include "search/tt.h"

#include <gtest/gtest.h>

using namespace chess;

TEST(TranspositionTable, ReturnsWhatWasStored) {
    TranspositionTable table(1);
    Move move(E2, E4, MoveKind::DoublePush);
    table.store(0x1234, 5, -42, Bound::Lower, move);

    const TableEntry* entry = table.probe(0x1234);
    ASSERT_NE(entry, nullptr);
    EXPECT_EQ(entry->move, move);
    EXPECT_EQ(entry->score, -42);
    EXPECT_EQ(entry->depth, 5);
    EXPECT_EQ(entry->bound, Bound::Lower);
}

TEST(TranspositionTable, FindsNothingForAnotherPosition) {
    TranspositionTable table(1);
    EXPECT_EQ(table.probe(0x1234), nullptr);

    //  Same slot (same low bits), different position
    table.store(0x1234, 5, 0, Bound::Exact, Move(E2, E4));
    EXPECT_EQ(table.probe(0x1234 + (uint64_t(1) << 40)), nullptr);
}

TEST(TranspositionTable, SizeIsAPowerOfTwoThatFits) {
    TranspositionTable table(1);
    EXPECT_EQ(table.capacity(), 1024u * 1024 / sizeof(TableEntry));
}

TEST(TranspositionTable, KeepsTheDeeperSearchOfTheSamePosition) {
    TranspositionTable table(1);
    table.store(0x1234, 6, 10, Bound::Exact, Move(E2, E4));
    table.store(0x1234, 3, 99, Bound::Exact, Move(D2, D4));
    EXPECT_EQ(table.probe(0x1234)->depth, 6);

    //  Another position on the same slot always replaces it, the newer one is more useful
    uint64_t other = 0x1234 + (uint64_t(1) << 40);
    table.store(other, 1, 0, Bound::Exact, Move(D2, D4));
    EXPECT_NE(table.probe(other), nullptr);
    EXPECT_EQ(table.probe(0x1234), nullptr);
}

TEST(TranspositionTable, FailLowKeepsTheEarlierBestMove) {
    TranspositionTable table(1);
    table.store(0x1234, 2, 10, Bound::Exact, Move(E2, E4));
    table.store(0x1234, 4, -5, Bound::Upper, Move());
    EXPECT_EQ(table.probe(0x1234)->move, Move(E2, E4));
}

TEST(TranspositionTable, ClearRemovesEverything) {
    TranspositionTable table(1);
    table.store(0x1234, 5, 0, Bound::Exact, Move(E2, E4));
    table.clear();
    EXPECT_EQ(table.probe(0x1234), nullptr);
}

TEST(TranspositionTable, MateScoresAreCountedFromTheStoredPosition) {
    //  Mate in 5 plies from the root, found at ply 2: mate in 3 from that position
    int score = MateScore - 5;
    EXPECT_EQ(scoreToTable(score, 2), MateScore - 3);
    EXPECT_EQ(scoreFromTable(MateScore - 3, 2), score);

    //  Reached at ply 4 by another move order, it is mate in 7 from the root
    EXPECT_EQ(scoreFromTable(scoreToTable(score, 2), 4), MateScore - 7);

    EXPECT_EQ(scoreToTable(-(MateScore - 5), 2), -(MateScore - 3));
    EXPECT_EQ(scoreToTable(150, 2), 150);  // other scores do not depend on the ply
}
