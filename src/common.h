// MasChess - Common types and utilities
#pragma once

#include <cstdint>
#include <cstddef>
#include <array>
#include <string>

using Bitboard = uint64_t;

enum Color : int { WHITE = 0, BLACK = 1, NO_COLOR = 2 };

enum PieceType : int { PAWN = 0, KNIGHT = 1, BISHOP = 2, ROOK = 3, QUEEN = 4, KING = 5, NO_PIECE_TYPE = 6 };

enum Square : int {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8, SQ_NONE
};

constexpr int FILE_OF(Square s) { return static_cast<int>(s) & 7; }
constexpr int RANK_OF(Square s) { return static_cast<int>(s) >> 3; }

constexpr Bitboard ONE = 1ULL;
constexpr Bitboard BB_EMPTY = 0ULL;
constexpr Bitboard BB_FULL  = ~0ULL;

constexpr Bitboard bit(Square s) { return ONE << static_cast<int>(s); }

inline int popcount(Bitboard b) {
    return __builtin_popcountll(b);
}

inline Square lsb(Bitboard b) {
    return static_cast<Square>(__builtin_ctzll(b));
}

inline Bitboard poplsb(Bitboard &b) {
    Bitboard l = b & -b;
    b ^= l;
    return l;
}

inline Square pop_lsb_sq(Bitboard &b) {
    Square s = lsb(b);
    b &= b - 1;
    return s;
}

inline Color opposite(Color c) { return c == WHITE ? BLACK : WHITE; }

// Move encoding: 32-bit
// bits 0..5  : from (0..63)
// bits 6..11  : to (0..63)
// bits 12..14 : promo (0..5; 0 means none)
// bits 15..18 : captured (0..6)
// bits 19..21 : piece (0..5)
// bits 22..25 : flags
// flags: 1<<0 = capture, 1<<1 = enpassant, 1<<2 = castle, 1<<3 = doublePush, 1<<4 = promo

using Move = uint32_t;

namespace MoveFlag {
    enum : uint32_t {
        CAPTURE = 1u << 22,
        ENPASS  = 1u << 23,
        CASTLE  = 1u << 24,
        DPP     = 1u << 25,
        PROMO   = 1u << 26
    };
}

inline Move make_move(int from, int to, int piece, int captured, int promo, uint32_t flags) {
    return (static_cast<Move>(from)) |
           (static_cast<Move>(to) << 6) |
           (static_cast<Move>(promo) << 12) |
           (static_cast<Move>(captured) << 15) |
           (static_cast<Move>(piece) << 19) |
           (flags);
}

inline int from_sq(Move m) { return m & 0x3F; }
inline int to_sq(Move m) { return (m >> 6) & 0x3F; }
inline int promo_of(Move m) { return (m >> 12) & 0x7; }
inline int captured_of(Move m) { return (m >> 15) & 0x7; }
inline int piece_of(Move m) { return (m >> 19) & 0x7; }
inline uint32_t flags_of(Move m) { return m & (MoveFlag::CAPTURE | MoveFlag::ENPASS | MoveFlag::CASTLE | MoveFlag::DPP | MoveFlag::PROMO); }

inline bool is_capture(Move m) { return flags_of(m) & MoveFlag::CAPTURE; }
inline bool is_promo(Move m) { return flags_of(m) & MoveFlag::PROMO; }
inline bool is_enpassant(Move m) { return flags_of(m) & MoveFlag::ENPASS; }
inline bool is_castle(Move m) { return flags_of(m) & MoveFlag::CASTLE; }

struct StateInfo {
    uint64_t zobrist;
    int castlingRights;
    int epSquare; // -1 if none
    int halfmoveClock;
    int capturedPiece; // PieceType or NO_PIECE_TYPE
    Move move;
};

// Castling rights bits: (white KQ black kq) -> 1,2,4,8
namespace Castle {
    enum : int { WK = 1, WQ = 2, BK = 4, BQ = 8 };
}

inline std::string square_to_string(Square s) {
    if (s == SQ_NONE) return "-";
    char file = 'a' + FILE_OF(s);
    char rank = '1' + RANK_OF(s);
    return std::string{file} + std::string{rank};
}

inline int char_to_file(char c) { return c - 'a'; }
inline int char_to_rank(char c) { return c - '1'; }

inline Square make_square(int file, int rank) { return static_cast<Square>((rank << 3) | file); }

