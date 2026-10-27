

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

//  UCI notation, promotions get the piece as a suffix (e7e8q) so all four promotions are distinct

std::string Move::moveToAlbr(){
    std::string uci = toAlgebraic(start) + toAlgebraic(target);

    switch(type){
        case N_PROMOTION: case N_PROMOTION_CAPTURE: uci += 'n'; break;
        case B_PROMOTION: case B_PROMOTION_CAPTURE: uci += 'b'; break;
        case R_PROMOTION: case R_PROMOTION_CAPTURE: uci += 'r'; break;
        case Q_PROMOTION: case Q_PROMOTION_CAPTURE: uci += 'q'; break;
    }
    return uci;
}
