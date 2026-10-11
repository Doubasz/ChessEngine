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
        unsigned int capturedPiece;
        bool pieceNeverMoved;

    
        Move();
        Move(int t, Position s, Position targ) : type(t), start(s), target(targ){}
        Move(int t, Position s, Position targ, unsigned int capture) : type(t), start(s), target(targ), capturedPiece(capture){}
        Move(int t, Position s, Position targ, bool nvmov) : type(t), start(s), target(targ), pieceNeverMoved(nvmov){}
        Move(int t, Position s, Position targ, unsigned int capture, bool nvmov) : type(t), start(s), target(targ), capturedPiece(capture), pieceNeverMoved(nvmov){}

    
        bool operator== (const Move& m) const;
        std::string moveToAlbr();
};

std::ostream& operator<<(std::ostream& os, const Move& p);


#endif