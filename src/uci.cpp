#include "uci.h"
#include <iostream>
#include <sstream>
#include <algorithm>

UCI::UCI() {}

void UCI::cmd_uci() {
    std::cout << "id name MasChess" << std::endl;
    std::cout << "id author OpenAI" << std::endl;
    std::cout << "option name Hash type spin default 64 min 1 max 2048" << std::endl;
    std::cout << "uciok" << std::endl;
}

void UCI::cmd_isready() { std::cout << "readyok" << std::endl; }

void UCI::cmd_ucinewgame() {
    searcher.new_game();
}

static inline int promo_char_to_piece(char c) {
    switch (tolower(c)) {
        case 'q': return QUEEN;
        case 'r': return ROOK;
        case 'b': return BISHOP;
        case 'n': return KNIGHT;
    }
    return QUEEN;
}

bool UCI::parse_move_str(const std::string &mstr, Move &outMove) {
    if (mstr.size() < 4) return false;
    int ffile = mstr[0] - 'a';
    int frank = mstr[1] - '1';
    int tfile = mstr[2] - 'a';
    int trank = mstr[3] - '1';
    if (ffile<0||ffile>7||tfile<0||tfile>7||frank<0||frank>7||trank<0||trank>7) return false;
    Square from = make_square(ffile, frank);
    Square to   = make_square(tfile, trank);
    int promo = 0;
    if (mstr.size() >= 5) promo = promo_char_to_piece(mstr[4]);

    std::vector<Move> legal; legal.reserve(256);
    board.generate_legal_moves(legal);
    for (Move m : legal) {
        if (from_sq(m)==from && to_sq(m)==to) {
            if (is_promo(m)) {
                if (promo == 0) continue;
                if (promo_of(m) != promo) continue;
            }
            outMove = m; return true;
        }
    }
    return false;
}

void UCI::cmd_position(const std::string &args) {
    std::istringstream ss(args);
    std::string tok; ss >> tok;
    if (tok == "startpos") {
        board.set_startpos();
        if (ss >> tok && tok == "moves") {
            std::string m;
            while (ss >> m) {
                Move mv; if (parse_move_str(m, mv)) { StateInfo st{}; board.make_move(mv, st); } else break;
            }
        }
    } else if (tok == "fen") {
        std::string fen, x;
        // FEN has up to 6 space-separated parts
        fen.clear();
        for (int i=0;i<6 && (ss >> x); ++i) {
            if (!fen.empty()) fen += ' ';
            fen += x;
        }
        board.set_fen(fen);
        if (ss >> tok && tok == "moves") {
            std::string m;
            while (ss >> m) {
                Move mv; if (parse_move_str(m, mv)) { StateInfo st{}; board.make_move(mv, st); } else break;
            }
        }
    }
}

void UCI::cmd_setoption(const std::string &args) {
    std::istringstream ss(args);
    std::string name, tok;
    ss >> tok; // name
    if (tok != "name") return;
    ss >> name;
    std::string value;
    while (ss >> tok && tok != "value") {}
    std::getline(ss, value);
    if (!value.empty() && value[0]==' ') value.erase(0,1);
    if (name == "Hash") {
        int mb = std::max(1, std::min(2048, std::stoi(value)));
        searcher.set_tt_mb(mb);
    }
}

void UCI::cmd_go(const std::string &args) {
    SearchLimits limits{};
    std::istringstream ss(args);
    std::string tok;
    while (ss >> tok) {
        if (tok == "wtime") ss >> limits.wtime;
        else if (tok == "btime") ss >> limits.btime;
        else if (tok == "winc") ss >> limits.winc;
        else if (tok == "binc") ss >> limits.binc;
        else if (tok == "movetime") ss >> limits.movetime;
        else if (tok == "depth") ss >> limits.depth;
        else if (tok == "nodes") ss >> limits.nodes;
        else if (tok == "infinite") { limits.wtime = limits.btime = limits.movetime = 0; }
    }
    auto res = searcher.search(board, limits);
    std::cout << "bestmove " << square_to_string((Square)from_sq(res.best))
              << square_to_string((Square)to_sq(res.best));
    if (is_promo(res.best)) {
        int p = promo_of(res.best);
        char pc = (p==QUEEN?'q': p==ROOK?'r': p==BISHOP?'b':'n');
        std::cout << pc;
    }
    std::cout << std::endl;
}

void UCI::loop() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "uci") cmd_uci();
        else if (line == "isready") cmd_isready();
        else if (line.rfind("setoption", 0) == 0) cmd_setoption(line.substr(9));
        else if (line == "ucinewgame") cmd_ucinewgame();
        else if (line.rfind("position", 0) == 0) cmd_position(line.substr(9));
        else if (line.rfind("go", 0) == 0) cmd_go(line.substr(2));
        else if (line == "stop") searcher.stop();
        else if (line == "quit") break;
    }
}

