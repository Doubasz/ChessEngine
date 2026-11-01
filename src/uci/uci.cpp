#include "uci/uci.h"

#include <algorithm>
#include <sstream>

namespace chess {

namespace {

//  Kept off every move's time budget: the GUI needs a moment to receive the move and stop the clock
constexpr long long MoveOverheadMs = 50;

//  Moves the remaining time is shared between, when the GUI does not say how many are left
constexpr long long DefaultMovesToGo = 30;

std::string infoLine(const SearchResult& r) {
    long long ms = r.elapsed.count();
    unsigned long long nps = ms > 0 ? r.nodes * 1000 / static_cast<unsigned long long>(ms) : 0;

    std::ostringstream line;
    line << "info depth " << r.depth;
    if (isMateScore(r.score)) line << " score mate " << mateInMoves(r.score);
    else line << " score cp " << r.score;
    line << " nodes " << r.nodes << " nps " << nps << " time " << ms << " pv " << r.bestMove.toUci();
    return line.str();
}

}  // namespace


//  With a clock, each move gets an equal share of the remaining time plus most of the increment,
//  never more than what is left on the clock

SearchLimits limitsFromGo(const std::string& args, Color us) {
    std::istringstream tokens(args);
    SearchLimits limits;

    long long time[2] = {-1, -1}, increment[2] = {0, 0};
    long long moveTime = -1, movesToGo = 0;

    //  infinite, and what is not supported (ponder, nodes, mate, searchmoves), searches without a limit
    std::string token;
    while (tokens >> token) {
        if (token == "depth") tokens >> limits.depth;
        else if (token == "movetime") tokens >> moveTime;
        else if (token == "wtime") tokens >> time[White];
        else if (token == "btime") tokens >> time[Black];
        else if (token == "winc") tokens >> increment[White];
        else if (token == "binc") tokens >> increment[Black];
        else if (token == "movestogo") tokens >> movesToGo;
    }

    long long budget = 0;
    if (moveTime >= 0) {
        budget = moveTime - MoveOverheadMs;
    } else if (time[us] >= 0) {
        long long movesLeft = movesToGo > 0 ? movesToGo : DefaultMovesToGo;
        budget = std::min(time[us] / movesLeft + increment[us] * 3 / 4, time[us] - MoveOverheadMs);
    } else {
        return limits;
    }

    limits.time = std::chrono::milliseconds(std::max(budget, 1LL));
    return limits;
}


UciEngine::UciEngine(std::ostream& out) : out_(out) {}

UciEngine::~UciEngine() {
    stopSearch();
}

bool UciEngine::handle(const std::string& line) {
    std::istringstream tokens(line);
    std::string command, args;
    tokens >> command;
    std::getline(tokens >> std::ws, args);

    if (command == "uci") onUci();
    else if (command == "isready") send("readyok");
    else if (command == "ucinewgame") {
        stopSearch();
        searcher_.clearTable();
        board_ = Board();
    }
    else if (command == "position") {
        stopSearch();
        onPosition(args);
    }
    else if (command == "go") onGo(args);
    else if (command == "stop") stopSearch();
    else if (command == "setoption") onSetOption(args);
    else if (command == "quit") {
        stopSearch();
        return false;
    }
    return true;
}

void UciEngine::waitForSearch() {
    if (infinite_) stopSearch();  // it would never end by itself
    else if (searchThread_.joinable()) searchThread_.join();
}


void UciEngine::onUci() {
    send("id name ChessEngine");
    send("id author Zendef");
    send("option name Hash type spin default " + std::to_string(TranspositionTable::DefaultSizeMb) +
         " min 1 max " + std::to_string(MaxHashMb));
    send("uciok");
}

//  position startpos [moves e2e4 e7e5 ...]
//  position fen <the 6 FEN fields> [moves ...]
//  The moves are played on the board rather than starting from the last position, so the board
//  knows the positions of the game and can see repetitions.

void UciEngine::onPosition(const std::string& args) {
    std::istringstream tokens(args);
    std::string token;
    tokens >> token;

    Board board;
    if (token == "startpos") {
        tokens >> token;
    } else if (token == "fen") {
        std::string fen;
        while (tokens >> token && token != "moves") fen += (fen.empty() ? "" : " ") + token;
        if (!board.setFen(fen)) {
            send("info string invalid fen: " + fen);
            return;
        }
    } else {
        return;
    }

    if (token == "moves") {
        while (tokens >> token) {
            Move m = board.moveFromUci(token);
            if (m.isNull()) {
                send("info string illegal move: " + token);
                break;
            }
            board.makeMove(m);
        }
    }
    board_ = board;
}

//  The search sends an info line after every depth, then bestmove. An infinite search (go infinite)
//  must not send bestmove before stop, even when it is over early (a mate found): it waits.

void UciEngine::onGo(const std::string& args) {
    stopSearch();

    std::istringstream tokens(args);
    std::string token;
    infinite_ = false;
    while (tokens >> token) {
        if (token == "infinite") infinite_ = true;
    }

    SearchLimits limits = limitsFromGo(args, board_.sideToMove());
    bool infinite = infinite_;

    stopRequested_ = false;
    searchDone_ = false;
    searchThread_ = std::thread([this, board = board_, limits, infinite] {
        SearchResult result = searcher_.search(board, limits, [this](const SearchResult& r) { send(infoLine(r)); });

        while (infinite && !stopRequested_) std::this_thread::sleep_for(std::chrono::milliseconds(1));

        send("bestmove " + result.bestMove.toUci());  // 0000 if there is no legal move
        searchDone_ = true;
    });
}

//  setoption name Hash value 64

void UciEngine::onSetOption(const std::string& args) {
    std::istringstream tokens(args);
    std::string token, name;
    tokens >> token;
    if (token != "name") return;

    while (tokens >> token && token != "value") name += (name.empty() ? "" : " ") + token;

    if (name == "Hash") {
        int mb = 0;
        if (!(tokens >> mb)) return;

        stopSearch();
        searcher_.setTableSize(size_t(std::clamp(mb, 1, MaxHashMb)));
    }
}

void UciEngine::stopSearch() {
    if (!searchThread_.joinable()) return;

    //  search() clears the stop request when it begins, so a stop sent before the thread got
    //  there would be lost: keep asking until the search has returned
    stopRequested_ = true;
    while (!searchDone_) {
        searcher_.stop();
        std::this_thread::yield();
    }
    searchThread_.join();
    infinite_ = false;
}

void UciEngine::send(const std::string& line) {
    std::lock_guard<std::mutex> lock(outMutex_);
    out_ << line << std::endl;  // flushed, the GUI reads each line as soon as it is written
}

}  // namespace chess
