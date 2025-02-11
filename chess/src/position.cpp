
#include "position.h"


Position::Position(int a, int b) : x(a), y(b) {}
Position::Position() : x(0), y(0) {}

bool Position::operator== (const Position& other) const{
    return x == other.x && y == other.y;
}

std::ostream& operator<<(std::ostream& os, const Position& p) {
    os << "(" << p.x << ", " << p.y << ")";
    return os; // Return the stream to allow chaining
} 

Position* concat(Position p1[], Position p2[], int size1, int size2){

    int size = size1 + size2;
    Position* temp = new Position[size];

    int i = 0, j = 0;
    int d = 0;
    while (d < size){
        if(i < size1){
            temp[d] = p1[i];
            i++;
        }
        else{
            temp[d] = p2[j];
            j++;
        }
        d++;
    }
    return temp;
}

void displayTab(std::vector<Position> tab){
    if(tab.empty()){
        std::cout << "empty" << std::endl;
        return;
    } 

    std::cout << "legalMoves: ";
    for(size_t i = 0; i < tab.size(); i++){
        std::cout << tab[i];
        if(i != tab.size() - 1){
            std::cout << ", ";
        }
    }
    std::cout << "\n";
}


std::string toAlgebraic(Position pos){
    std::string ret = "";
    ret += pos.y + 'a';
    ret += '8' - pos.x;

    return ret;
}