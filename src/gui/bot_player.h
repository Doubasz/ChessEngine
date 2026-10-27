#ifndef BOT_PLAYER_H
#define BOT_PLAYER_H

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include "core/board.h"
#include "search/search.h"

//  Runs the search on its own thread so the window keeps drawing while the bot thinks.
//  The GUI starts it, polls isDone() every frame, and takes the result once it is there.

class BotPlayer{
    public:
        BotPlayer() = default;
        ~BotPlayer();

        BotPlayer(const BotPlayer&) = delete;
        BotPlayer& operator=(const BotPlayer&) = delete;

        //  Starts thinking about a copy of board, cancelling any search still running
        void start(const chess::Board& board, const chess::SearchLimits& limits);

        //  Stops the search and waits for its thread, the result is dropped
        void cancel();

        bool isThinking() const;  // started and the result not taken yet
        bool isDone() const;  // the result is ready for takeResult

        //  The final result, once isDone(). The bot is idle afterwards.
        chess::SearchResult takeResult();

        //  While thinking: the last completed depth, with the nodes and the time so far
        chess::SearchResult progress() const;

        std::chrono::milliseconds elapsed() const;  // since start

    private:
        chess::Searcher searcher;
        std::thread worker;
        std::chrono::steady_clock::time_point startTime;

        mutable std::mutex mutex;  // guards latest, written by the worker
        chess::SearchResult latest;
        std::atomic<bool> done{false};
};

#endif
