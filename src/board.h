#pragma once

#include "common.h"
#include <array>
#include <vector>
#include <string>

struct ZobristKeys {
    std::array<std::array<std::array<uint64_t, 64>, 6>, 2> psq;
    std::array<uint64_t, 16> castling;
    std::array<uint64_t, 8> epFile;
    uint64_t side;
};

class Board {
public:
    Board();

    void set_startpos();
    bool set_fen(const std::string &fen);
    std::string get_fen() const;

    void set_side(Color c) { sideToMove = c; }
    Color side() const { return sideToMove; }

    Bitboard pieces(Color c, PieceType pt) const { return pieceBB[c][pt]; }
    Bitboard color_bb(Color c) const { return occBB[c]; }
    Bitboard occ() const { return occBB[WHITE] | occBB[BLACK]; }

    int castling_rights() const { return castlingRights; }
    int ep_square() const { return epSquare; }
    uint64_t key() const { return zobrist; }
    int halfmove() const { return halfmoveClock; }
    int fullmove() const { return fullmoveNumber; }

    bool is_square_attacked(Square s, Color by) const;
    bool in_check(Color c) const;

    bool make_move(Move m, StateInfo &st);
    void unmake_move(const StateInfo &st);

    void generate_legal_moves(std::vector<Move> &out) const;
    void generate_captures(std::vector<Move> &out) const;

    int piece_on(Square s, Color &cOut, PieceType &ptOut) const;

    const ZobristKeys &zkeys() const { return zkeys_; }

private:
    friend struct MoveGen;

    std::array<std::array<Bitboard, 6>, 2> pieceBB{};
    std::array<Bitboard, 2> occBB{};

    Color sideToMove{WHITE};
    int castlingRights{0};
    int epSquare{-1};
    int halfmoveClock{0};
    int fullmoveNumber{1};
    uint64_t zobrist{0};

    ZobristKeys zkeys_{};

    void clear();
    void put_piece(Color c, PieceType pt, Square s);
    void remove_piece(Color c, PieceType pt, Square s);
    void move_piece(Color c, PieceType pt, Square from, Square to);
    void update_zobrist_piece(Color c, PieceType pt, Square s);
    void init_zobrist();
};

