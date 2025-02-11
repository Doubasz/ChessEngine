#ifndef POSITION_H
#define POSITION_H

#include <iostream>
#include <vector>


struct Position{
    int x, y;

    Position(int a, int b);
    Position();

    bool operator== (const Position& other) const;

    
};

// Overload the << operator
std::ostream& operator<<(std::ostream& os, const Position& p);

Position* concat(Position p1[], Position p2[], int size1, int size2);
void displayTab(std::vector<Position> tab);
std::string toAlgebraic(Position pos);


#endif