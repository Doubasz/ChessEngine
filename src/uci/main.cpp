#include "uci/uci.h"

#include <iostream>
#include <string>

//  The engine for chess GUIs: add this program as a UCI engine in Arena, Cute Chess,
//  BanksiaGUI... It reads the GUI's commands on stdin and answers on stdout.
//
//  It can also be driven by hand or from a script:
//      printf "position startpos\ngo depth 6\n" | chess-uci

int main() {
    chess::UciEngine engine(std::cout);

    std::string line;
    while (std::getline(std::cin, line) && engine.handle(line)) {}

    //  The input ended without quit (a script): let the last search finish
    engine.waitForSearch();
    return 0;
}
