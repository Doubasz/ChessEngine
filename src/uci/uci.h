#pragma once

#include "core/board.h"
#include "search/search.h"

#include <atomic>
#include <mutex>
#include <ostream>
#include <string>
#include <thread>

namespace chess {

//  The engine side of the UCI protocol (https://www.chessprogramming.org/UCI), which lets chess
//  GUIs like Arena or Cute Chess play with the engine. The GUI starts the engine as a separate
//  program and sends one command per line on its standard input, the engine answers on its
//  standard output:
//
//      uci                                 -> id name ..., option ..., uciok
//      isready                             -> readyok
//      ucinewgame
//      position startpos moves e2e4 e7e5   (or position fen <fen> moves ...)
//      go wtime 60000 btime 60000          -> info depth ... (one per depth), then bestmove g1f3
//      stop                                -> bestmove ... (ends the running search)
//      setoption name Hash value 64
//      quit
//
//  The search runs on its own thread, so commands like stop and isready are answered while it
//  thinks. Unknown commands are ignored, as the protocol asks.
class UciEngine {
public:
    static constexpr int MaxHashMb = 1024;

    explicit UciEngine(std::ostream& out);
    ~UciEngine();  // stops the running search

    UciEngine(const UciEngine&) = delete;
    UciEngine& operator=(const UciEngine&) = delete;

    //  Handles one line from the GUI. Returns false after quit.
    bool handle(const std::string& line);

    //  Blocks until the running search ends by itself (a go with a depth or time limit).
    //  An infinite search is stopped instead, it would never end.
    void waitForSearch();

private:
    void onUci();
    void onPosition(const std::string& args);
    void onGo(const std::string& args);
    void onSetOption(const std::string& args);
    void stopSearch();

    void send(const std::string& line);  // one whole line, safe from both threads

    std::ostream& out_;
    std::mutex outMutex_;

    Board board_;
    Searcher searcher_;
    std::thread searchThread_;
    std::atomic<bool> searchDone_{true};
    std::atomic<bool> stopRequested_{false};  // stop was received, an infinite search may send bestmove
    bool infinite_ = false;  // the running search is a go infinite
};

//  The search limits of a go command, args being what follows "go". us is the side to move,
//  whose clock (wtime or btime) counts.
SearchLimits limitsFromGo(const std::string& args, Color us);

}  // namespace chess
