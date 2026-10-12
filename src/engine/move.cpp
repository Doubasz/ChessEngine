

#include "move.h"


Move::Move(){
    type = -1;
    start = {-1, -1};
    target = {-1, -1};
}

bool Move::operator== (const Move& m) const{
    return (type == m.type && start == m.start && target == m.target); 
}


std::ostream& operator<<(std::ostream& os, const Move& m) {
    os << "(" << m.type << ", " << m.start << m.target << ")";
    return os; // Return the stream to allow chaining
} 

std::string Move::moveToAlbr(){
    return toAlgebraic(start) + toAlgebraic(target); 
}
