#include "board.h"
#include <random>
#include <sstream>
#include <algorithm>

static Bitboard KNIGHT_ATTACKS[64];
static Bitboard KING_ATTACKS[64];
static Bitboard PAWN_ATTACKS[2][64];

static void init_attacks() {
    static bool inited = false;
    if (inited) return;
    inited = true;

    for (int s = 0; s < 64; ++s) {
        int f = s & 7, r = s >> 3;
        Bitboard b = 0;
        auto add = [&](int df, int dr){ int nf=f+df, nr=r+dr; if (nf>=0&&nf<8&&nr>=0&&nr<8) b |= ONE << (nr*8+nf); };
        add(+1,+2); add(+2,+1); add(+2,-1); add(+1,-2); add(-1,-2); add(-2,-1); add(-2,+1); add(-1,+2);
        KNIGHT_ATTACKS[s] = b;

        b = 0;
        auto addk = [&](int df, int dr){ int nf=f+df, nr=r+dr; if (nf>=0&&nf<8&&nr>=0&&nr<8) b |= ONE << (nr*8+nf); };
        for (int df=-1; df<=1; ++df)
            for (int dr=-1; dr<=1; ++dr)
                if (df!=0 || dr!=0) addk(df,dr);
        KING_ATTACKS[s] = b;

        Bitboard w=0,bk=0;
        if (r<7) { if (f>0) w |= ONE << ((r+1)*8 + (f-1)); if (f<7) w |= ONE << ((r+1)*8 + (f+1)); }
        if (r>0) { if (f>0) bk |= ONE << ((r-1)*8 + (f-1)); if (f<7) bk |= ONE << ((r-1)*8 + (f+1)); }
        PAWN_ATTACKS[WHITE][s] = w;
        PAWN_ATTACKS[BLACK][s] = bk;
    }
}

Board::Board() {
    init_zobrist();
    init_attacks();
    set_startpos();
}

void Board::init_zobrist() {
    std::mt19937_64 rng(0xC001D00DFEEDBEEFULL);
    auto rand64 = [&]() { return (rng() << 1) ^ rng(); };

    for (int c=0;c<2;++c)
        for (int p=0;p<6;++p)
            for (int s=0;s<64;++s)
                zkeys_.psq[c][p][s] = rand64();
    for (int i=0;i<16;++i) zkeys_.castling[i] = rand64();
    for (int i=0;i<8;++i) zkeys_.epFile[i] = rand64();
    zkeys_.side = rand64();
}

void Board::clear() {
    for (int c=0;c<2;++c) {
        for (int p=0;p<6;++p) pieceBB[c][p] = 0ULL;
        occBB[c] = 0ULL;
    }
    castlingRights = 0;
    epSquare = -1;
    halfmoveClock = 0;
    fullmoveNumber = 1;
    zobrist = 0ULL;
    sideToMove = WHITE;
}

void Board::put_piece(Color c, PieceType pt, Square s) {
    pieceBB[c][pt] |= bit(s);
    occBB[c] |= bit(s);
    zobrist ^= zkeys_.psq[c][pt][s];
}

void Board::remove_piece(Color c, PieceType pt, Square s) {
    pieceBB[c][pt] &= ~bit(s);
    occBB[c] &= ~bit(s);
    zobrist ^= zkeys_.psq[c][pt][s];
}

void Board::move_piece(Color c, PieceType pt, Square from, Square to) {
    pieceBB[c][pt] ^= bit(from);
    pieceBB[c][pt] |= bit(to);
    occBB[c] ^= bit(from);
    occBB[c] |= bit(to);
    zobrist ^= zkeys_.psq[c][pt][from];
    zobrist ^= zkeys_.psq[c][pt][to];
}

void Board::set_startpos() {
    clear();
    const char *fen = "rnbrqkbn/pppppppp/8/8/8/8/PPPPPPPP/RNBRQKBN w KQkq - 0 1";
    // Correct startpos FEN is below; above was temporary to init; we'll set via set_fen
    set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN w KQkq - 0 1");
}

bool Board::set_fen(const std::string &fen) {
    clear();
    std::istringstream ss(fen);
    std::string board, stm, castle, ep;
    int half=0, full=1;
    if (!(ss >> board >> stm >> castle >> ep >> half >> full)) {
        // some GUIs omit clocks
        ss.clear(); ss.str(fen);
        if (!(ss >> board >> stm >> castle >> ep)) return false;
    }

    int idx = 56; // start from A8
    for (char c : board) {
        if (c == '/') { idx -= 16; continue; }
        if (c >= '1' && c <= '9') { idx += (c - '0'); continue; }
        Color col = isupper(c) ? WHITE : BLACK;
        char lc = static_cast<char>(tolower(c));
        PieceType pt = NO_PIECE_TYPE;
        switch (lc) {
            case 'p': pt = PAWN; break;
            case 'n': pt = KNIGHT; break;
            case 'b': pt = BISHOP; break;
            case 'r': pt = ROOK; break;
            case 'q': pt = QUEEN; break;
            case 'k': pt = KING; break;
            default: return false;
        }
        put_piece(col, pt, static_cast<Square>(idx));
        idx++;
    }

    sideToMove = (stm == "w") ? WHITE : BLACK;
    if (sideToMove == BLACK) zobrist ^= zkeys_.side;

    castlingRights = 0;
    if (castle.find('K') != std::string::npos) castlingRights |= Castle::WK;
    if (castle.find('Q') != std::string::npos) castlingRights |= Castle::WQ;
    if (castle.find('k') != std::string::npos) castlingRights |= Castle::BK;
    if (castle.find('q') != std::string::npos) castlingRights |= Castle::BQ;
    zobrist ^= zkeys_.castling[castlingRights];

    if (ep != "-") {
        int f = char_to_file(ep[0]);
        int r = char_to_rank(ep[1]);
        epSquare = static_cast<int>(make_square(f, r));
        zobrist ^= zkeys_.epFile[f];
    } else epSquare = -1;

    halfmoveClock = half;
    fullmoveNumber = full;
    return true;
}

std::string Board::get_fen() const {
    std::string s;
    for (int r = 7; r >= 0; --r) {
        int empty = 0;
        for (int f = 0; f < 8; ++f) {
            Square sq = static_cast<Square>(r*8+f);
            Color c; PieceType pt;
            if (piece_on(sq, c, pt)) {
                if (empty) { s += char('0' + empty); empty = 0; }
                char ch = '1';
                switch (pt) {
                    case PAWN: ch='p';break; case KNIGHT: ch='n';break; case BISHOP: ch='b';break;
                    case ROOK: ch='r';break; case QUEEN: ch='q';break; case KING: ch='k';break;
                    default: break;
                }
                if (c == WHITE) ch = static_cast<char>(toupper(ch));
                s += ch;
            } else ++empty;
        }
        if (empty) s += char('0' + empty);
        if (r) s += '/';
    }
    s += sideToMove == WHITE ? " w " : " b ";
    std::string cast;
    if (castlingRights & Castle::WK) cast += 'K';
    if (castlingRights & Castle::WQ) cast += 'Q';
    if (castlingRights & Castle::BK) cast += 'k';
    if (castlingRights & Castle::BQ) cast += 'q';
    s += cast.empty() ? "- " : (cast + " ");
    if (epSquare >= 0) s += square_to_string(static_cast<Square>(epSquare)) + " "; else s += "- ";
    s += std::to_string(halfmoveClock) + " " + std::to_string(fullmoveNumber);
    return s;
}

int Board::piece_on(Square sq, Color &cOut, PieceType &ptOut) const {
    Bitboard m = bit(sq);
    for (int c=0;c<2;++c)
        for (int p=0;p<6;++p)
            if (pieceBB[c][p] & m) { cOut = (Color)c; ptOut = (PieceType)p; return 1; }
    return 0;
}

bool Board::is_square_attacked(Square s, Color by) const {
    // Pawns
    if (PAWN_ATTACKS[by][s] & pieceBB[by][PAWN]) return true;
    // Knights
    if (KNIGHT_ATTACKS[s] & pieceBB[by][KNIGHT]) return true;
    // Kings
    if (KING_ATTACKS[s] & pieceBB[by][KING]) return true;
    // Bishops/Queens (diagonals)
    Bitboard occAll = occ();
    int dirsD[4][2] = {{1,1},{1,-1},{-1,1},{-1,-1}};
    for (auto &d : dirsD) {
        int f = FILE_OF(s), r = RANK_OF(s);
        while (true) {
            f += d[0]; r += d[1];
            if (f<0||f>7||r<0||r>7) break;
            Square ns = static_cast<Square>(r*8+f);
            Bitboard m = bit(ns);
            if (pieceBB[by][BISHOP] & m) return true;
            if (pieceBB[by][QUEEN] & m) return true;
            if (occAll & m) break;
        }
    }
    // Rooks/Queens (orthogonals)
    int dirsO[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (auto &d : dirsO) {
        int f = FILE_OF(s), r = RANK_OF(s);
        while (true) {
            f += d[0]; r += d[1];
            if (f<0||f>7||r<0||r>7) break;
            Square ns = static_cast<Square>(r*8+f);
            Bitboard m = bit(ns);
            if (pieceBB[by][ROOK] & m) return true;
            if (pieceBB[by][QUEEN] & m) return true;
            if (occAll & m) break;
        }
    }
    return false;
}

bool Board::in_check(Color c) const {
    Bitboard kingBB = pieceBB[c][KING];
    if (!kingBB) return false;
    Square ks = lsb(kingBB);
    return is_square_attacked(ks, opposite(c));
}

static inline void add_move(std::vector<Move> &out, int from, int to, int piece, int captured, int promo, uint32_t flags) {
    out.push_back(make_move(from, to, piece, captured, promo, flags));
}

static void generate_pseudo(const Board &b, std::vector<Move> &out, bool capturesOnly) {
    Color us = b.side();
    Color them = opposite(us);
    Bitboard occUs = b.color_bb(us);
    Bitboard occThem = b.color_bb(them);
    Bitboard occAll = b.occ();

    // Pawns
    Bitboard pawns = b.pieces(us, PAWN);
    while (pawns) {
        Square from = pop_lsb_sq(pawns);
        int f = FILE_OF(from), r = RANK_OF(from);
        int dir = us == WHITE ? 1 : -1;
        int startRank = us == WHITE ? 1 : 6;
        int promoRank = us == WHITE ? 6 : 1;
        // Pushes
        if (!capturesOnly) {
            int nr = r + dir;
            if (nr>=0 && nr<8) {
                Square to = static_cast<Square>(nr*8+f);
                if (!(occAll & bit(to))) {
                    if (r == promoRank) {
                        add_move(out, from, to, PAWN, NO_PIECE_TYPE, QUEEN, MoveFlag::PROMO);
                        add_move(out, from, to, PAWN, NO_PIECE_TYPE, ROOK, MoveFlag::PROMO);
                        add_move(out, from, to, PAWN, NO_PIECE_TYPE, BISHOP, MoveFlag::PROMO);
                        add_move(out, from, to, PAWN, NO_PIECE_TYPE, KNIGHT, MoveFlag::PROMO);
                    } else {
                        add_move(out, from, to, PAWN, NO_PIECE_TYPE, 0, 0);
                        if (r == startRank) {
                            int nr2 = r + 2*dir;
                            Square to2 = static_cast<Square>(nr2*8+f);
                            if (!(occAll & bit(to2))) add_move(out, from, to2, PAWN, NO_PIECE_TYPE, 0, MoveFlag::DPP);
                        }
                    }
                }
            }
        }
        // Captures and en passant
        Bitboard atks = PAWN_ATTACKS[us][from] & occThem;
        while (atks) {
            Square to = pop_lsb_sq(atks);
            Color c; PieceType pt; b.piece_on(to, c, pt);
            if (r == promoRank) {
                add_move(out, from, to, PAWN, pt, QUEEN, MoveFlag::CAPTURE | MoveFlag::PROMO);
                add_move(out, from, to, PAWN, pt, ROOK, MoveFlag::CAPTURE | MoveFlag::PROMO);
                add_move(out, from, to, PAWN, pt, BISHOP, MoveFlag::CAPTURE | MoveFlag::PROMO);
                add_move(out, from, to, PAWN, pt, KNIGHT, MoveFlag::CAPTURE | MoveFlag::PROMO);
            } else {
                add_move(out, from, to, PAWN, pt, 0, MoveFlag::CAPTURE);
            }
        }
        if (b.ep_square() >= 0) {
            Square epsq = static_cast<Square>(b.ep_square());
            Bitboard epMask = PAWN_ATTACKS[us][from] & bit(epsq);
            if (epMask) add_move(out, from, epsq, PAWN, PAWN, 0, MoveFlag::CAPTURE | MoveFlag::ENPASS);
        }
    }

    // Knights
    Bitboard knights = b.pieces(us, KNIGHT);
    while (knights) {
        Square from = pop_lsb_sq(knights);
        Bitboard moves = KNIGHT_ATTACKS[from] & ~occUs;
        while (moves) {
            Square to = pop_lsb_sq(moves);
            int cap = (occThem & bit(to)) ? (int)NO_PIECE_TYPE : -1;
            if (cap >= 0) {
                Color c; PieceType pt; b.piece_on(to, c, pt);
                add_move(out, from, to, KNIGHT, pt, 0, MoveFlag::CAPTURE);
            } else if (!capturesOnly) {
                add_move(out, from, to, KNIGHT, NO_PIECE_TYPE, 0, 0);
            }
        }
    }

    // Bishops, Rooks, Queens via rays
    auto gen_sliders = [&](PieceType pt, const int dirs[][2], int numDirs){
        Bitboard bb = b.pieces(us, pt);
        while (bb) {
            Square from = pop_lsb_sq(bb);
            int f0 = FILE_OF(from), r0 = RANK_OF(from);
            for (int i=0;i<numDirs;++i){
                int f=f0, r=r0;
                while (true){
                    f += dirs[i][0]; r += dirs[i][1];
                    if (f<0||f>7||r<0||r>7) break;
                    Square to = static_cast<Square>(r*8+f);
                    Bitboard m = bit(to);
                    if (occUs & m) break;
                    if (occThem & m) {
                        Color c; PieceType cap; b.piece_on(to, c, cap);
                        add_move(out, from, to, pt, cap, 0, MoveFlag::CAPTURE);
                        break;
                    }
                    if (!capturesOnly) add_move(out, from, to, pt, NO_PIECE_TYPE, 0, 0);
                }
            }
        }
    };
    const int dirsB[4][2]={{1,1},{1,-1},{-1,1},{-1,-1}};
    const int dirsR[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
    gen_sliders(BISHOP, dirsB, 4);
    gen_sliders(ROOK, dirsR, 4);
    // Queens combine
    Bitboard queens = b.pieces(us, QUEEN);
    while (queens) {
        Square from = pop_lsb_sq(queens);
        int f0 = FILE_OF(from), r0 = RANK_OF(from);
        for (auto d: dirsB) {
            int f=f0, r=r0; while(true){ f+=d[0]; r+=d[1]; if(f<0||f>7||r<0||r>7) break; Square to=static_cast<Square>(r*8+f); Bitboard m=bit(to); if (occUs&m) break; if (occThem&m){ Color c; PieceType cap; b.piece_on(to,c,cap); add_move(out, from,to, QUEEN, cap, 0, MoveFlag::CAPTURE); break;} if (!capturesOnly) add_move(out, from,to, QUEEN, NO_PIECE_TYPE, 0, 0);} }
        for (auto d: dirsR) {
            int f=f0, r=r0; while(true){ f+=d[0]; r+=d[1]; if(f<0||f>7||r<0||r>7) break; Square to=static_cast<Square>(r*8+f); Bitboard m=bit(to); if (occUs&m) break; if (occThem&m){ Color c; PieceType cap; b.piece_on(to,c,cap); add_move(out, from,to, QUEEN, cap, 0, MoveFlag::CAPTURE); break;} if (!capturesOnly) add_move(out, from,to, QUEEN, NO_PIECE_TYPE, 0, 0);} }
    }

    // King moves + castling
    Bitboard king = b.pieces(us, KING);
    if (king) {
        Square from = lsb(king);
        Bitboard moves = KING_ATTACKS[from] & ~occUs;
        while (moves) {
            Square to = pop_lsb_sq(moves);
            int cap = (occThem & bit(to)) ? 1 : 0;
            if (cap) { Color c; PieceType pt; b.piece_on(to, c, pt); add_move(out, from,to, KING, pt, 0, MoveFlag::CAPTURE);} else if (!capturesOnly) add_move(out, from,to, KING, NO_PIECE_TYPE, 0, 0);
        }
        if (!capturesOnly) {
            bool inCheck = b.in_check(us);
            if (!inCheck) {
                // Castling rights
                if (us==WHITE) {
                    if (b.castling_rights() & Castle::WK) {
                        if (!(b.occ() & (bit(F1)|bit(G1))) && !b.is_square_attacked(F1, them) && !b.is_square_attacked(G1, them))
                            add_move(out, E1, G1, KING, NO_PIECE_TYPE, 0, MoveFlag::CASTLE);
                    }
                    if (b.castling_rights() & Castle::WQ) {
                        if (!(b.occ() & (bit(D1)|bit(C1)|bit(B1))) && !b.is_square_attacked(D1, them) && !b.is_square_attacked(C1, them))
                            add_move(out, E1, C1, KING, NO_PIECE_TYPE, 0, MoveFlag::CASTLE);
                    }
                } else {
                    if (b.castling_rights() & Castle::BK) {
                        if (!(b.occ() & (bit(F8)|bit(G8))) && !b.is_square_attacked(F8, them) && !b.is_square_attacked(G8, them))
                            add_move(out, E8, G8, KING, NO_PIECE_TYPE, 0, MoveFlag::CASTLE);
                    }
                    if (b.castling_rights() & Castle::BQ) {
                        if (!(b.occ() & (bit(D8)|bit(C8)|bit(B8))) && !b.is_square_attacked(D8, them) && !b.is_square_attacked(C8, them))
                            add_move(out, E8, C8, KING, NO_PIECE_TYPE, 0, MoveFlag::CASTLE);
                    }
                }
            }
        }
    }
}

void Board::generate_legal_moves(std::vector<Move> &out) const {
    std::vector<Move> moves; moves.reserve(256);
    generate_pseudo(*this, moves, false);
    // Filter illegal
    Board copy = *this;
    StateInfo st{};
    for (Move m : moves) {
        if (copy.make_move(m, st)) out.push_back(m);
        copy.unmake_move(st);
    }
}

void Board::generate_captures(std::vector<Move> &out) const {
    std::vector<Move> moves; moves.reserve(128);
    generate_pseudo(*this, moves, true);
    Board copy = *this;
    StateInfo st{};
    for (Move m : moves) {
        if (copy.make_move(m, st)) out.push_back(m);
        copy.unmake_move(st);
    }
}

bool Board::make_move(Move m, StateInfo &st) {
    st.zobrist = zobrist;
    st.castlingRights = castlingRights;
    st.epSquare = epSquare;
    st.halfmoveClock = halfmoveClock;
    st.capturedPiece = captured_of(m);
    st.move = m;

    int from = from_sq(m);
    int to = to_sq(m);
    PieceType piece = (PieceType)piece_of(m);
    PieceType captured = (PieceType)captured_of(m);
    PieceType promo = (PieceType)promo_of(m);
    uint32_t fl = flags_of(m);

    // Update side key
    zobrist ^= zkeys_.side;

    // EP hash out
    if (epSquare >= 0) zobrist ^= zkeys_.epFile[FILE_OF((Square)epSquare)];
    epSquare = -1;

    // Move piece
    move_piece(sideToMove, piece, (Square)from, (Square)to);

    // Captures
    if (fl & MoveFlag::CAPTURE) {
        if (fl & MoveFlag::ENPASS) {
            int dir = sideToMove == WHITE ? -1 : 1;
            Square capSq = static_cast<Square>((RANK_OF((Square)to)+dir)*8 + FILE_OF((Square)to));
            remove_piece(opposite(sideToMove), PAWN, capSq);
        } else {
            remove_piece(opposite(sideToMove), captured, (Square)to);
        }
        halfmoveClock = 0;
    } else {
        // pawn moves reset 50-move rule
        if (piece == PAWN) halfmoveClock = 0; else ++halfmoveClock;
    }

    // Promotion
    if (fl & MoveFlag::PROMO) {
        remove_piece(sideToMove, PAWN, (Square)to);
        put_piece(sideToMove, promo, (Square)to);
    }

    // Double pawn push sets ep
    if (fl & MoveFlag::DPP) {
        int dir = sideToMove == WHITE ? 1 : -1;
        Square eps = static_cast<Square>((RANK_OF((Square)to)-dir)*8 + FILE_OF((Square)to));
        epSquare = eps;
        zobrist ^= zkeys_.epFile[FILE_OF(eps)];
    }

    // Castling move: move rook
    if (fl & MoveFlag::CASTLE) {
        if (to == G1) { move_piece(WHITE, ROOK, H1, F1); }
        else if (to == C1) { move_piece(WHITE, ROOK, A1, D1); }
        else if (to == G8) { move_piece(BLACK, ROOK, H8, F8); }
        else if (to == C8) { move_piece(BLACK, ROOK, A8, D8); }
    }

    // Update castling rights and hash
    zobrist ^= zkeys_.castling[castlingRights];
    auto revoke = [&](Square s){
        if (s==A1) castlingRights &= ~Castle::WQ;
        if (s==H1) castlingRights &= ~Castle::WK;
        if (s==E1) castlingRights &= ~(Castle::WK|Castle::WQ);
        if (s==A8) castlingRights &= ~Castle::BQ;
        if (s==H8) castlingRights &= ~Castle::BK;
        if (s==E8) castlingRights &= ~(Castle::BK|Castle::BQ);
    };
    revoke((Square)from);
    revoke((Square)to);
    zobrist ^= zkeys_.castling[castlingRights];

    // Update side
    sideToMove = opposite(sideToMove);
    if (sideToMove == WHITE) ++fullmoveNumber;

    // legality: king not in check
    if (in_check(opposite(sideToMove))) return true;
    // Undo if illegal
    unmake_move(st);
    return false;
}

void Board::unmake_move(const StateInfo &st) {
    // Restore basics
    sideToMove = opposite(sideToMove);
    zobrist = st.zobrist;
    castlingRights = st.castlingRights;
    epSquare = st.epSquare;
    halfmoveClock = st.halfmoveClock;
    if (sideToMove == BLACK) --fullmoveNumber;

    Move m = st.move;
    int from = from_sq(m), to = to_sq(m);
    PieceType piece = (PieceType)piece_of(m);
    PieceType captured = (PieceType)captured_of(m);
    uint32_t fl = flags_of(m);
    PieceType promo = (PieceType)promo_of(m);

    // Undo promotion
    if (fl & MoveFlag::PROMO) {
        remove_piece(sideToMove, promo, (Square)to);
        put_piece(sideToMove, PAWN, (Square)to);
    }
    // Move piece back
    move_piece(sideToMove, (fl & MoveFlag::PROMO) ? PAWN : piece, (Square)to, (Square)from);
    // Restore rook if castle
    if (fl & MoveFlag::CASTLE) {
        if (to == G1) { move_piece(WHITE, ROOK, F1, H1); }
        else if (to == C1) { move_piece(WHITE, ROOK, D1, A1); }
        else if (to == G8) { move_piece(BLACK, ROOK, F8, H8); }
        else if (to == C8) { move_piece(BLACK, ROOK, D8, A8); }
    }
    // Restore captured
    if (fl & MoveFlag::CAPTURE) {
        if (fl & MoveFlag::ENPASS) {
            int dir = sideToMove == WHITE ? -1 : 1;
            Square capSq = static_cast<Square>((RANK_OF((Square)to)+dir)*8 + FILE_OF((Square)to));
            put_piece(opposite(sideToMove), PAWN, capSq);
        } else {
            put_piece(opposite(sideToMove), captured, (Square)to);
        }
    }
}

