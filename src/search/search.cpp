#include "search/search.h"
#include "search/eval.h"

#include <algorithm>

namespace chess {

SearchResult Searcher::search(const Board& board, const SearchLimits& limits, const IterationCallback& onIteration) {
    board_ = board;
    nodes_ = 0;
    completedDepth_ = 0;
    stopRequested_ = false;
    stopped_ = false;
    start_ = std::chrono::steady_clock::now();
    timeLimit_ = limits.time;
    killers_ = {};

    SearchResult result;

    MoveList moves;
    board_.generateLegalMoves(moves);
    if (moves.empty()) {
        result.score = board_.inCheck() ? -MateScore : 0;
        return result;
    }
    result.bestMove = moves[0];

    int maxDepth = std::clamp(limits.depth, 1, MaxPly - 1);
    for (int depth = 1; depth <= maxDepth; depth++) {
        Move best = result.bestMove;  // searched first, it was the best one at the previous depth
        int score = searchRoot(depth, best);
        if (stopped_) break;  // this iteration is incomplete, keep the previous one

        completedDepth_ = depth;
        result.bestMove = best;
        result.score = score;
        result.depth = depth;
        result.nodes = nodes_;
        result.elapsed = elapsed();
        if (onIteration) onIteration(result);

        //  A forced mate either way, searching deeper cannot change the outcome
        if (isMateScore(score)) break;
    }

    result.nodes = nodes_;
    result.elapsed = elapsed();
    return result;
}

//  Like negamax, but it keeps track of which move gave the best score.
//  bestMove comes in as the move to try first and goes out as the best move found.

int Searcher::searchRoot(int depth, Move& bestMove) {
    MoveList moves;
    board_.generateLegalMoves(moves);

    std::array<Move, MoveList::Capacity> ordered;
    orderMoves(moves, ordered, 0, bestMove);

    int alpha = -Infinity;
    const int beta = Infinity;

    for (size_t i = 0; i < moves.size(); i++) {
        board_.makeMove(ordered[i]);
        int score = -negamax(depth - 1, -beta, -alpha, 1);
        board_.unmakeMove();

        if (stopped_) return 0;

        if (score > alpha) {
            alpha = score;
            bestMove = ordered[i];
        }
    }
    return alpha;
}

//  The score of the position searched depth plies deep, clamped to [alpha, beta] (fail-hard):
//  alpha is what the side to move is already sure to get elsewhere, beta what the opponent is.
//  As soon as a move reaches beta the opponent will avoid this position, the other moves are skipped.

int Searcher::negamax(int depth, int alpha, int beta, int ply) {
    if (shouldStop()) return 0;
    if (ply >= MaxPly - 1) return evaluate(board_);
    if (board_.halfmoveClock() >= 100) return 0;

    //  Check extension: never stop the search right after a check, the reply may lose material or be mate
    bool inCheck = board_.inCheck();
    if (inCheck) depth++;

    if (depth <= 0) return quiescence(alpha, beta, ply);
    nodes_++;

    MoveList moves;
    board_.generateLegalMoves(moves);
    if (moves.empty()) return inCheck ? -MateScore + ply : 0;

    std::array<Move, MoveList::Capacity> ordered;
    orderMoves(moves, ordered, ply, Move());

    for (size_t i = 0; i < moves.size(); i++) {
        Move m = ordered[i];
        bool quiet = !isCapture(m) && m.promotion() == NoPieceType;

        board_.makeMove(m);
        int score = -negamax(depth - 1, -beta, -alpha, ply + 1);
        board_.unmakeMove();

        if (stopped_) return 0;

        if (score >= beta) {
            if (quiet && killers_[ply][0] != m) {
                killers_[ply][1] = killers_[ply][0];
                killers_[ply][0] = m;
            }
            return beta;
        }
        alpha = std::max(alpha, score);
    }
    return alpha;
}

//  At the end of the main search, keep playing captures until the position is quiet, otherwise
//  the evaluation could be taken in the middle of an exchange (a queen just took a pawn defended
//  by another pawn). The side to move may also decline every capture and keep the static
//  evaluation (stand pat), unless it is in check: then every move is searched.

int Searcher::quiescence(int alpha, int beta, int ply) {
    if (shouldStop()) return 0;
    nodes_++;
    if (ply >= MaxPly - 1) return evaluate(board_);

    bool inCheck = board_.inCheck();
    if (!inCheck) {
        int standPat = evaluate(board_);
        if (standPat >= beta) return beta;
        alpha = std::max(alpha, standPat);
    }

    MoveList moves;
    board_.generateLegalMoves(moves);
    if (moves.empty()) return inCheck ? -MateScore + ply : 0;

    std::array<Move, MoveList::Capacity> ordered;
    orderMoves(moves, ordered, ply, Move());

    for (size_t i = 0; i < moves.size(); i++) {
        Move m = ordered[i];
        if (!inCheck) {
            bool queenPromotion = m.promotion() == Queen;
            if (!isCapture(m) && !queenPromotion) continue;
            if (m.promotion() != NoPieceType && !queenPromotion) continue;  // underpromotions only matter in rare cases
        }

        board_.makeMove(m);
        int score = -quiescence(-beta, -alpha, ply + 1);
        board_.unmakeMove();

        if (stopped_) return 0;

        if (score >= beta) return beta;
        alpha = std::max(alpha, score);
    }
    return alpha;
}



// ---- Helpers ------------------------------------------------------------------

//  Good moves first, so alpha-beta cuts off sooner: first, then captures of the most valuable
//  piece by the least valuable one (MVV-LVA), then promotions, then the killer moves of this ply.

void Searcher::orderMoves(const MoveList& moves, std::array<Move, MoveList::Capacity>& ordered, int ply, Move first) const {
    std::array<int, MoveList::Capacity> scores;

    for (size_t i = 0; i < moves.size(); i++) {
        Move m = moves[i];
        int score = 0;

        if (m == first) {
            score = 1'000'000;
        } else if (isCapture(m)) {
            PieceType victim = m.kind() == MoveKind::EnPassant ? Pawn : typeOf(board_.pieceAt(m.to()));
            PieceType attacker = typeOf(board_.pieceAt(m.from()));
            score = 100'000 + pieceValue(victim) * 10 - attacker;
        } else if (m.promotion() != NoPieceType) {
            score = 90'000;
        } else if (m == killers_[ply][0]) {
            score = 80'000;
        } else if (m == killers_[ply][1]) {
            score = 70'000;
        }
        if (m.promotion() != NoPieceType) score += pieceValue(m.promotion());

        //  Insertion sort, highest score first. At most a few dozen moves, faster than std::sort.
        size_t j = i;
        while (j > 0 && scores[j - 1] < score) {
            scores[j] = scores[j - 1];
            ordered[j] = ordered[j - 1];
            j--;
        }
        scores[j] = score;
        ordered[j] = m;
    }
}

bool Searcher::isCapture(Move m) const {
    return board_.pieceAt(m.to()) != NoPiece || m.kind() == MoveKind::EnPassant;
}

std::chrono::milliseconds Searcher::elapsed() const {
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start_);
}

//  Checked at every node. The clock is only read every 2048 nodes, reading it is not free.
//  Never stops during depth 1, so there is always a complete iteration to return.

bool Searcher::shouldStop() {
    if (stopped_) return true;
    if (completedDepth_ == 0) return false;

    if (stopRequested_.load(std::memory_order_relaxed)) stopped_ = true;
    else if (timeLimit_.count() > 0 && (nodes_ & 2047) == 0 && elapsed() >= timeLimit_) stopped_ = true;

    return stopped_;
}

}  // namespace chess
