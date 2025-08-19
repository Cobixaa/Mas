#include "eval.h"

static const int PIECE_VAL[6] = {100, 320, 330, 500, 900, 20000};

static const int PST_PAWN[64] = {
      0,  0,  0,  0,  0,  0,  0,  0,
     60, 60, 60, 60, 60, 60, 60, 60,
     15, 20, 25, 30, 30, 25, 20, 15,
     10, 12, 15, 20, 20, 15, 12, 10,
      5,  8, 10, 15, 15, 10,  8,  5,
      2,  4,  6, 10, 10,  6,  4,  2,
      1,  1,  2,  3,  3,  2,  1,  1,
      0,  0,  0,  0,  0,  0,  0,  0
};

static const int PST_KNIGHT[64] = {
    -30,-20,-10, -8, -8,-10,-20,-30,
    -20,  0,  5,  8,  8,  5,  0,-20,
    -10,  8, 12, 15, 15, 12,  8,-10,
     -8,  8, 15, 18, 18, 15,  8, -8,
     -8,  8, 15, 18, 18, 15,  8, -8,
    -10,  8, 12, 15, 15, 12,  8,-10,
    -20,  0,  5,  8,  8,  5,  0,-20,
    -30,-20,-10, -8, -8,-10,-20,-30
};

static const int PST_BISHOP[64] = {
    -10, -5, -5, -5, -5, -5, -5,-10,
     -5,  5,  3,  5,  5,  3,  5, -5,
     -5,  3, 10, 12, 12, 10,  3, -5,
     -5,  5, 12, 15, 15, 12,  5, -5,
     -5,  5, 12, 15, 15, 12,  5, -5,
     -5,  3, 10, 12, 12, 10,  3, -5,
     -5,  5,  3,  5,  5,  3,  5, -5,
    -10, -5, -5, -5, -5, -5, -5,-10
};

static const int PST_ROOK[64] = {
      0,  0,  2,  3,  3,  2,  0,  0,
      2,  4,  6,  8,  8,  6,  4,  2,
      2,  4,  6,  8,  8,  6,  4,  2,
      2,  4,  6,  8,  8,  6,  4,  2,
      2,  4,  6,  8,  8,  6,  4,  2,
      2,  4,  6,  8,  8,  6,  4,  2,
      2,  4,  6,  8,  8,  6,  4,  2,
      0,  0,  2,  3,  3,  2,  0,  0
};

static const int PST_QUEEN[64] = {
     -5, -2, -2, -1, -1, -2, -2, -5,
     -2,  0,  0,  1,  1,  0,  0, -2,
     -2,  0,  1,  1,  1,  1,  0, -2,
     -1,  1,  1,  1,  1,  1,  1, -1,
     -1,  1,  1,  1,  1,  1,  1, -1,
     -2,  0,  1,  1,  1,  1,  0, -2,
     -2,  0,  0,  1,  1,  0,  0, -2,
     -5, -2, -2, -1, -1, -2, -2, -5
};

static const int PST_KING_MG[64] = {
     -3, -4, -4, -5, -5, -4, -4, -3,
     -3, -4, -4, -5, -5, -4, -4, -3,
     -3, -4, -4, -5, -5, -4, -4, -3,
     -3, -4, -4, -5, -5, -4, -4, -3,
     -2, -3, -3, -4, -4, -3, -3, -2,
     -1, -2, -2, -2, -2, -2, -2, -1,
      2,  2,  0,  0,  0,  0,  2,  2,
      2,  3,  1,  0,  0,  1,  3,  2
};

static inline int pst_mirror(int idx) {
    int f = idx & 7, r = idx >> 3;
    int mr = 7 - r;
    return mr*8 + f;
}

int evaluate(const Board &b) {
    // Side-to-move perspective
    int score = 0;
    for (int c=0;c<2;++c) {
        int sign = (c==b.side()) ? 1 : -1;
        for (int p=0;p<6;++p) {
            Bitboard bb = b.pieces((Color)c, (PieceType)p);
            while (bb) {
                Square s = pop_lsb_sq(bb);
                int idx = (c==WHITE) ? s : pst_mirror(s);
                switch (p) {
                    case PAWN:   score += sign * (PIECE_VAL[p] + PST_PAWN[idx]); break;
                    case KNIGHT: score += sign * (PIECE_VAL[p] + PST_KNIGHT[idx]); break;
                    case BISHOP: score += sign * (PIECE_VAL[p] + PST_BISHOP[idx]); break;
                    case ROOK:   score += sign * (PIECE_VAL[p] + PST_ROOK[idx]); break;
                    case QUEEN:  score += sign * (PIECE_VAL[p] + PST_QUEEN[idx]); break;
                    case KING:   score += sign * (PIECE_VAL[p] + PST_KING_MG[idx]); break;
                    default: break;
                }
            }
        }
    }
    // Bishop pair bonus
    for (int c=0;c<2;++c) {
        if (popcount(b.pieces((Color)c, BISHOP)) >= 2)
            score += (c==b.side() ? 35 : -35);
    }
    return score;
}

