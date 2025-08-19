#include "search.h"
#include "eval.h"
#include <algorithm>

Searcher::Searcher() {
    tt.resize_mb(64);
}

void Searcher::set_tt_mb(int mb) { tt.resize_mb((size_t)mb); }
void Searcher::new_game() { tt.clear(); }

void Searcher::update_time(const Board &b, const SearchLimits &limits) {
    startTime = std::chrono::steady_clock::now();
    if (limits.movetime > 0) {
        timeLimitMs = limits.movetime;
        return;
    }
    int remain = b.side() == WHITE ? limits.wtime : limits.btime;
    int inc = b.side() == WHITE ? limits.winc : limits.binc;
    if (remain > 0) {
        // Allocate a small slice of available time + a portion of increment
        int alloc = std::max(10, remain / 30) + (inc / 2);
        timeLimitMs = alloc;
    } else timeLimitMs = 0; // unlimited
}

bool Searcher::time_up() const {
    if (timeLimitMs <= 0) return false;
    auto now = std::chrono::steady_clock::now();
    int elapsed = (int)std::chrono::duration_cast<std::chrono::milliseconds>(now - startTime).count();
    return elapsed >= timeLimitMs;
}

int Searcher::mvv_lva(Move m, const Board &b) const {
    int victim = captured_of(m);
    int attacker = piece_of(m);
    static const int val[7] = {100, 320, 330, 500, 900, 20000, 0};
    return val[victim] * 10 - val[attacker];
}

void Searcher::order_moves(Board &b, std::vector<Move> &moves, Move ttMove, Move parentMove, int ply) {
    std::stable_sort(moves.begin(), moves.end(), [&](Move a, Move c){
        if (a == ttMove) return true;
        if (c == ttMove) return false;
        bool capA = is_capture(a), capC = is_capture(c);
        if (capA != capC) return capA; // captures first
        if (capA) return mvv_lva(a, b) > mvv_lva(c, b);
        int ha = history[piece_of(a)&1][from_sq(a)][to_sq(a)];
        int hc = history[piece_of(c)&1][from_sq(c)][to_sq(c)];
        if (ha != hc) return ha > hc;
        if (a == killerMoves[0][ply]) return true;
        if (c == killerMoves[0][ply]) return false;
        if (a == killerMoves[1][ply]) return true;
        if (c == killerMoves[1][ply]) return false;
        return false;
    });
}

int Searcher::qsearch(Board &b, int alpha, int beta, int ply) {
    if (time_up()) stopFlag = true;
    if (stopFlag) return 0;
    nodes++;

    int stand = evaluate(b);
    if (stand >= beta) return stand;
    if (stand > alpha) alpha = stand;

    std::vector<Move> moves; moves.reserve(64);
    b.generate_captures(moves);
    order_moves(b, moves, 0, 0, ply);

    for (size_t i=0;i<moves.size();++i) {
        StateInfo st{};
        if (!b.make_move(moves[i], st)) continue;
        int score = -qsearch(b, -beta, -alpha, ply+1);
        b.unmake_move(st);
        if (score >= beta) return score;
        if (score > alpha) alpha = score;
    }
    return alpha;
}

int Searcher::search_impl(Board &b, int depth, int alpha, int beta, int ply, bool cutNode, Move parentMove) {
    if (time_up()) stopFlag = true;
    if (stopFlag) return 0;
    bool inCheck = b.in_check(b.side());
    if (depth <= 0) return qsearch(b, alpha, beta, ply);

    nodes++;

    // Mate distance pruning
    const int MATE = 32000;
    alpha = std::max(alpha, -MATE + ply);
    beta  = std::min(beta,  MATE - ply - 1);
    if (alpha >= beta) return alpha;

    // TT probe
    TTEntry te{};
    Move ttMove = 0;
    if (tt.probe(b.key(), te) && te.depth >= depth) {
        int tts = score_from_tt(te.score, ply);
        if (te.flag == TT_EXACT) return tts;
        else if (te.flag == TT_ALPHA && tts <= alpha) return alpha;
        else if (te.flag == TT_BETA && tts >= beta) return beta;
        ttMove = te.bestMove;
    }

    std::vector<Move> moves; moves.reserve(256);
    b.generate_legal_moves(moves);
    if (moves.empty()) {
        if (inCheck) return -MATE + ply; // mate
        return 0; // stalemate
    }

    order_moves(b, moves, ttMove, parentMove, ply);

    int bestScore = -INF;
    Move bestMove = 0;
    int legalCount = 0;

    for (size_t i=0;i<moves.size(); ++i) {
        StateInfo st{};
        if (!b.make_move(moves[i], st)) continue;
        ++legalCount;

        int newDepth = depth - 1;
        int score;

        bool isCapture = is_capture(moves[i]);
        bool isCheck = b.in_check(b.side());
        bool quiet = !isCapture && !isCheck;

        if (legalCount > 1 && quiet && depth >= 3) {
            // LMR
            int R = 1 + (int)(i >= 4) + (cutNode ? 1 : 0);
            score = -search_impl(b, newDepth - R, -alpha-1, -alpha, ply+1, true, moves[i]);
            if (score > alpha) {
                score = -search_impl(b, newDepth, -alpha-1, -alpha, ply+1, true, moves[i]);
                if (score > alpha && score < beta) {
                    score = -search_impl(b, newDepth, -beta, -alpha, ply+1, false, moves[i]);
                }
            }
        } else {
            // PVS
            score = -search_impl(b, newDepth, -alpha-1, -alpha, ply+1, true, moves[i]);
            if (score > alpha && score < beta) {
                score = -search_impl(b, newDepth, -beta, -alpha, ply+1, false, moves[i]);
            }
        }

        b.unmake_move(st);

        if (score > bestScore) {
            bestScore = score;
            bestMove = moves[i];
            if (ply == 0) {
                // Capture PV at root by simple re-search not needed; will report later
            }
        }
        if (score > alpha) {
            alpha = score;
            // update history/killers for quiets
            if (quiet) {
                if (killerMoves[0][ply] != (int)moves[i]) {
                    killerMoves[1][ply] = killerMoves[0][ply];
                    killerMoves[0][ply] = (int)moves[i];
                }
                history[piece_of(moves[i])&1][from_sq(moves[i])][to_sq(moves[i])] += depth * depth;
            }
        }
        if (alpha >= beta) {
            // beta cutoff
            if (quiet) {
                if (killerMoves[0][ply] != (int)moves[i]) {
                    killerMoves[1][ply] = killerMoves[0][ply];
                    killerMoves[0][ply] = (int)moves[i];
                }
                history[piece_of(moves[i])&1][from_sq(moves[i])][to_sq(moves[i])] += depth * depth;
            }
            break;
        }
    }

    // store TT
    uint8_t flag = TT_EXACT;
    if (bestScore <= alpha) flag = TT_ALPHA; // after loop, alpha equals bestScore
    if (bestScore >= beta) flag = TT_BETA;
    int stt = score_to_tt(bestScore, ply);
    tt.store(b.key(), depth, stt, flag, bestMove);
    return bestScore;
}

SearchResult Searcher::search(Board &b, const SearchLimits &limits) {
    stopFlag = false;
    nodes = 0;
    memset(killerMoves, 0, sizeof(killerMoves));
    memset(history, 0, sizeof(history));
    update_time(b, limits);

    // Root moves
    rootMoves.clear();
    b.generate_legal_moves(rootMoves);
    if (rootMoves.empty()) {
        SearchResult r{}; r.best = 0; r.score = 0; return r;
    }

    int maxDepth = limits.depth > 0 ? limits.depth : 64;
    int alpha = -INF, beta = INF;
    Move bestMove = rootMoves[0];
    int bestScore = -INF;

    for (rootDepth = 1; rootDepth <= maxDepth; ++rootDepth) {
        // Aspiration window
        if (rootDepth > 4) { alpha = std::max(bestScore - 50, -INF); beta = std::min(bestScore + 50, INF); } else { alpha = -INF; beta = INF; }
        int score;
        while (true) {
            score = search_impl(b, rootDepth, alpha, beta, 0, false, 0);
            if (stopFlag) break;
            if (score <= alpha) { alpha -= 100; continue; }
            if (score >= beta)  { beta  += 100; continue; }
            break;
        }
        if (stopFlag) break;

        bestScore = score;
        // Pick best root move from TT
        TTEntry e{};
        if (tt.probe(b.key(), e)) bestMove = e.bestMove ? e.bestMove : bestMove;

        if (time_up()) break;
    }

    SearchResult res{};
    res.best = bestMove;
    res.score = bestScore;
    return res;
}

void Searcher::stop() { stopFlag = true; }

