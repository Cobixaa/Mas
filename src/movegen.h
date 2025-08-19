#pragma once

#include "board.h"
#include <vector>

struct MoveList {
    std::vector<Move> moves;
    void clear() { moves.clear(); }
    void add(Move m) { moves.push_back(m); }
    size_t size() const { return moves.size(); }
    Move operator[](size_t i) const { return moves[i]; }
};

