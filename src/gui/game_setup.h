#ifndef GAME_SETUP_H
#define GAME_SETUP_H

#include <chrono>
#include "core/types.h"
#include "search/search.h"

//  What the menu chose: who plays, and how strong the bot is

enum class Difficulty { Easy, Medium, Hard };

struct GameSetup{
    bool vsBot = false;
    chess::Color humanColor = chess::White;  // only used against the bot
    Difficulty difficulty = Difficulty::Medium;
};

inline const char* difficultyName(Difficulty d){
    switch(d){
        case Difficulty::Easy: return "Easy";
        case Difficulty::Medium: return "Medium";
        case Difficulty::Hard: return "Hard";
    }
    return "";
}

//  Easy and Medium look a fixed number of plies ahead, Hard goes as deep as it can in 3 seconds

inline chess::SearchLimits searchLimitsFor(Difficulty d){
    chess::SearchLimits limits;
    switch(d){
        case Difficulty::Easy: limits.depth = 2; break;
        case Difficulty::Medium: limits.depth = 4; break;
        case Difficulty::Hard: limits.time = std::chrono::seconds(3); break;
    }
    return limits;
}

#endif
