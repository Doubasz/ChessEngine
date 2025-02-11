#ifndef MOVE_H
#define MOVE_H

#include "position.h"

enum moveFlags {
    QUIET,
    D_PAWN_PUSH,
    KCASTLE,
    QCASTLE,
    CAPTURE,
    EP_CAPTURE,
    N_PROMOTION,
    B_PROMOTION,
    R_PROMOTION,
    Q_PROMOTION,
    N_PROMOTION_CAPTURE,
    B_PROMOTION_CAPTURE,
    R_PROMOTION_CAPTURE,
    Q_PROMOTION_CAPTURE,
};



class Move{
    public:
        int type;
        Position start;
        Position target;
        int capturedPiece;

    
        Move();
        Move(int t, Position s, Position targ) : type(t), start(s), target(targ){}
        Move(int t, Position s, Position targ, int capture) : type(t), start(s), target(targ), capturedPiece(capture){}

    
        bool operator== (const Move& m) const;
        std::string moveToAlbr();
};

std::ostream& operator<<(std::ostream& os, const Move& p);


#endif