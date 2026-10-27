#include "core/board.h"
#include "search/eval.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <string>

using namespace chess;

namespace {

Board boardFrom(const std::string& fen) {
    Board board;
    EXPECT_TRUE(board.setFen(fen)) << fen;
    return board;
}

//  The same position with the colors swapped: ranks upside down, white pieces become black
//  and the other side moves. Only for FENs without castling rights or en passant square.
std::string colorFlipped(const std::string& fen) {
    std::string placement = fen.substr(0, fen.find(' '));
    char side = fen[placement.size() + 1];

    std::string flipped;
    size_t end = placement.size();
    while (true) {
        size_t start = placement.rfind('/', end - 1);
        start = start == std::string::npos ? 0 : start + 1;
        if (!flipped.empty()) flipped += '/';
        flipped += placement.substr(start, end - start);
        if (start == 0) break;
        end = start - 1;
    }
    for (char& c : flipped) {
        if (std::isalpha(static_cast<unsigned char>(c))) {
            c = std::isupper(static_cast<unsigned char>(c)) ? char(std::tolower(c)) : char(std::toupper(c));
        }
    }
    return flipped + (side == 'w' ? " b" : " w") + " - - 0 1";
}

}  // namespace


TEST(Eval, StartingPositionIsEqual) {
    EXPECT_EQ(evaluate(Board()), 0);
}

TEST(Eval, SameScoreWithColorsSwapped) {
    for (const char* fen : {
             "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w - - 0 1",
             "4k3/8/8/3q4/8/8/8/3RK3 w - - 0 1",
             "8/5pk1/6p1/8/3K4/8/1P6/8 b - - 0 1",
         }) {
        EXPECT_EQ(evaluate(boardFrom(fen)), evaluate(boardFrom(colorFlipped(fen)))) << fen;
    }
}

TEST(Eval, ScoreIsFromTheSideToMove) {
    int white = evaluate(boardFrom("4k3/8/8/8/8/8/8/Q3K3 w - - 0 1"));
    int black = evaluate(boardFrom("4k3/8/8/8/8/8/8/Q3K3 b - - 0 1"));
    EXPECT_GT(white, 800);
    EXPECT_EQ(black, -white);
}

TEST(Eval, ExtraPieceOutweighsItsBadSquare) {
    //  Even badly placed in the corner, a knight up is worth far more than any square bonus
    EXPECT_GT(evaluate(boardFrom("4k3/8/8/8/8/8/8/N3K3 w - - 0 1")), 200);
    EXPECT_LT(evaluate(boardFrom("4k3/8/8/8/8/8/8/N3K3 b - - 0 1")), -200);
}

TEST(Eval, CentralKnightBeatsKnightOnTheRim) {
    EXPECT_GT(evaluate(boardFrom("4k3/8/8/8/3N4/8/8/4K3 w - - 0 1")),
              evaluate(boardFrom("4k3/8/8/8/8/8/8/N3K3 w - - 0 1")));
}

TEST(Eval, KingSeeksTheCenterInTheEndgame) {
    int central = evaluate(boardFrom("4k3/4p3/8/8/4K3/8/4P3/8 w - - 0 1"));
    int corner = evaluate(boardFrom("4k3/4p3/8/8/8/8/4P3/K7 w - - 0 1"));
    EXPECT_GT(central, corner);
}

TEST(Eval, KingStaysSafeInTheMiddlegame) {
    int castled = evaluate(boardFrom("r1bq1rk1/pppp1ppp/2n2n2/2b1p3/2B1P3/2N2N2/PPPP1PPP/R1BQ1RK1 w - - 0 1"));
    int center = evaluate(boardFrom("r1bq1rk1/pppp1ppp/2n2n2/2b1p3/2B1P3/2NK1N2/PPPP1PPP/R1BQ1R2 w - - 0 1"));
    EXPECT_GT(castled, center);
}
