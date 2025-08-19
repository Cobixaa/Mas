#pragma once

#include <cstdint>
#include <vector>
#include "common.h"

enum TTFlag : uint8_t { TT_EXACT = 0, TT_ALPHA = 1, TT_BETA = 2 };

struct TTEntry {
    uint64_t key = 0;
    int16_t score = 0;
    int8_t depth = -1;
    uint8_t flag = TT_ALPHA;
    Move bestMove = 0;
};

class TranspositionTable {
public:
    void resize_mb(size_t mb);
    void clear();
    void store(uint64_t key, int depth, int score, uint8_t flag, Move best);
    bool probe(uint64_t key, TTEntry &out) const;

private:
    std::vector<TTEntry> table;
    size_t mask = 0;
};

int score_to_tt(int score, int ply);
int score_from_tt(int score, int ply);

