#include "tt.h"
#include <cstring>

static constexpr int MATE_SCORE = 32000;
static constexpr int MATE_BOUND = 31000;

int score_to_tt(int score, int ply) {
    if (score > MATE_BOUND) return score + ply;
    if (score < -MATE_BOUND) return score - ply;
    return score;
}

int score_from_tt(int score, int ply) {
    if (score > MATE_BOUND) return score - ply;
    if (score < -MATE_BOUND) return score + ply;
    return score;
}

void TranspositionTable::resize_mb(size_t mb) {
    size_t bytes = mb * 1024ull * 1024ull;
    size_t n = bytes / sizeof(TTEntry);
    if (n == 0) n = 1;
    // round up to power of two
    size_t p = 1; while (p < n) p <<= 1;
    table.assign(p, {});
    mask = p - 1;
}

void TranspositionTable::clear() {
    std::fill(table.begin(), table.end(), TTEntry{});
}

void TranspositionTable::store(uint64_t key, int depth, int score, uint8_t flag, Move best) {
    if (table.empty()) return;
    size_t idx = key & mask;
    TTEntry &e = table[idx];
    if (e.depth <= depth || e.key != key) {
        e.key = key;
        e.depth = (int8_t)depth;
        e.score = (int16_t)score;
        e.flag = flag;
        e.bestMove = best;
    }
}

bool TranspositionTable::probe(uint64_t key, TTEntry &out) const {
    if (table.empty()) return false;
    const TTEntry &e = table[key & mask];
    if (e.key == key && e.depth >= 0) { out = e; return true; }
    return false;
}

