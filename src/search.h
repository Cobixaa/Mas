#pragma once

#include "board.h"
#include "tt.h"
#include <vector>
#include <atomic>
#include <chrono>

struct SearchLimits {
    int depth = 0;        // max depth (0 = auto)
    int movetime = 0;     // fixed time in ms
    int wtime = 0, btime = 0; // remaining time in ms
    int winc = 0, binc = 0;   // increment in ms
    int nodes = 0;        // node limit (0 = no limit)
};

struct SearchResult {
    Move best = 0;
    int score = 0;
    std::vector<Move> pv;
};

class Searcher {
public:
    Searcher();
    void set_tt_mb(int mb);
    void new_game();

    SearchResult search(Board &b, const SearchLimits &limits);
    void stop();

private:
    static constexpr int INF = 30000;
    static constexpr int MAX_PLY = 128;

    TranspositionTable tt;
    std::atomic<bool> stopFlag{false};
    std::atomic<uint64_t> nodes{0};

    int killerMoves[2][MAX_PLY]{}; // two slots per ply
    int history[2][64][64]{};      // piece from->to history heuristic

    std::chrono::steady_clock::time_point startTime;
    int timeLimitMs = 0;

    int rootDepth = 0;
    std::vector<Move> rootMoves;
    std::vector<Move> bestPV;

    void update_time(const Board &b, const SearchLimits &limits);
    bool time_up() const;

    int qsearch(Board &b, int alpha, int beta, int ply);
    int search_impl(Board &b, int depth, int alpha, int beta, int ply, bool cutNode, Move parentMove);
    void order_moves(Board &b, std::vector<Move> &moves, Move ttMove, Move parentMove, int ply);
    int mvv_lva(Move m, const Board &b) const;
};

