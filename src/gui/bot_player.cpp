#include "bot_player.h"
#include <algorithm>

BotPlayer::~BotPlayer(){
    cancel();
}

void BotPlayer::start(const chess::Board& board, const chess::SearchLimits& limits){
    cancel();

    latest = chess::SearchResult();
    done = false;
    startTime = std::chrono::steady_clock::now();

    worker = std::thread([this, board, limits](){
        chess::SearchResult result = searcher.search(board, limits, [this](const chess::SearchResult& iteration){
            std::lock_guard<std::mutex> lock(mutex);
            latest = iteration;
        });

        std::lock_guard<std::mutex> lock(mutex);
        latest = result;
        done = true;
    });
}

void BotPlayer::cancel(){
    if(!worker.joinable()) return;

    //  search() clears the stop request when it begins, so a stop sent before the thread got
    //  there would be lost: keep asking until the search has returned
    while(!done){
        searcher.stop();
        std::this_thread::yield();
    }
    worker.join();
}

bool BotPlayer::isThinking() const{
    return worker.joinable();
}

bool BotPlayer::isDone() const{
    return worker.joinable() && done;
}

chess::SearchResult BotPlayer::takeResult(){
    worker.join();
    return latest;
}

chess::SearchResult BotPlayer::progress() const{
    std::lock_guard<std::mutex> lock(mutex);
    chess::SearchResult result = latest;
    result.nodes = std::max(result.nodes, searcher.nodesSearched());
    result.elapsed = elapsed();
    return result;
}

std::chrono::milliseconds BotPlayer::elapsed() const{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime);
}
