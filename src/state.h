/*
 *  Game state
 */

#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "lowlevel.h"

typedef unsigned long long hash_t;

/* Players */
typedef enum player_e_ { WHITE = 0, BLACK, N_PLAYERS } player_e;

extern const char player_text[N_PLAYERS][6];

typedef unsigned char status_t;

/* clang-format off */

/* Location of a square on the board. A is queen's rook's file, 1 is white's
   back rank. */
typedef enum {
  NO_SQUARE = -1,
  A1 = 0,
      B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  N_SQUARES
} square_e;

/* clang-format on */

/* Number of ranks (horizontal) and files (vertical) on the board */
enum { N_RANKS = 8, N_FILES = 8 };

/* Sides of the board */
typedef enum boardside_e { QUEENSIDE = 0, KINGSIDE, BOTHSIDES, N_BOARDSIDE } boardside_e;

/* Bit set for castling rights */
typedef uint8_t castle_rights_t;
enum {
  WHITE_QUEENSIDE = 0x01,
  WHITE_KINGSIDE = 0x02,
  WHITE_BOTHSIDES = 0x03,
  BLACK_QUEENSIDE = 0x04,
  BLACK_KINGSIDE = 0x08,
  BLACK_BOTHSIDES = 0x0c,
  ALL_CASTLE_RIGHTS = 0x0f
};

/* Piece type */
typedef enum piece_e_ {
  EMPTY = -1,
  PAWN = 0,
  ROOK,
  KNIGHT,
  BISHOP,
  QUEEN,
  KING,
  N_PIECE_T
} piece_e;

enum {
  /* Number of pieces */
  N_PIECES = 32,
};

/* Bitboard - a 64-bit number describing a set of squares on the board */
typedef unsigned long long bitboard_t;

/*
 * The state of the game
 */

/* Four stacks are used to represent the same information with different bit
   orders, with the squares indexed according to horizontal, vertical and
   diagonal schemes. */

enum {
  /* Total number of bitboards in a stack - there is one bitboard for each type
    of piece for each player */
  N_PLANES = N_PIECE_T * N_PLAYERS,
};

/* Position, game state, and pre-calculated moves */
typedef struct state_s_ {
  /* The stacks */
  bitboard_t a[N_PLANES];          /* -  Horizontal    */
  bitboard_t b[N_PLANES];          /* |  Vertical      */
  bitboard_t c[N_PLANES];          /* /  Diagonal      */
  bitboard_t d[N_PLANES];          /* \  Diagonal      */
  bitboard_t player_a[N_PLAYERS];  /* Set of each players pieces */
  bitboard_t player_b[N_PLAYERS];  /* Set of each players pieces */
  bitboard_t player_c[N_PLAYERS];  /* Set of each players pieces */
  bitboard_t player_d[N_PLAYERS];  /* Set of each players pieces */
  bitboard_t total_a;              /* Set of all pieces */
  bitboard_t total_b;              /* Set of all pieces */
  bitboard_t total_c;              /* Set of all pieces */
  bitboard_t total_d;              /* Set of all pieces */
  bitboard_t moves[N_PIECES];      /* Set of squares each piece can move to */
  bitboard_t claim[N_PLAYERS];     /* Set of all squares each player can move to */
  square_e piece_square[N_PIECES]; /* Square location of each piece */
  int8_t piece_at[N_SQUARES];      /* Type of piece at each square */
  int8_t index_at[N_SQUARES];      /* Piece index at each board position */
  int halfmove;
  int fullmove;
  status_t turn : 1;         /* Player to move next */
  status_t check[N_PLAYERS]; /* Whether each player is in check */
  castle_rights_t castling_rights;
  bitboard_t en_passant; /* En-passant squares */
  hash_t hash;
  int ply;
} state_s;

/* Bit set for indicating result conditions */
typedef uint8_t moveresult_t;
enum {
  CAPTURED = 1 << 0,
  EN_PASSANT = 1 << 1,
  CHECK = 1 << 2,
  MATE = 1 << 3,
  SELF_CHECK = 1 << 4,
  CASTLED = 1 << 5,
  PROMOTED = 1 << 6,
};

/* A move */
typedef struct move_s_ {
  square_e from, to;
  piece_e piece;
  piece_e promotion;
  moveresult_t result;
} move_s;

/* Invalid move conditions */
enum {
  ERR_BASE = 0,        /* OK */
  ERR_NO_PIECE,        /* There is no piece at the "from" square */
  ERR_SRC_EQUAL_DEST,  /* The "from" and "to" squares are equal */
  ERR_NOT_MY_PIECE,    /* The piece at the "from" square belongs to the opponent */
  ERR_CANT_MOVE_THERE, /* The piece at the "from" square can't legally move to the "to" square */
  ERR_PROMOTION        /* Illegal promotion to a pawn */
};

extern bitboard_t *square2bit;
extern const piece_e piece_type[N_PLANES];
extern const player_e piece_player[N_PLANES];
extern const player_e opponent[N_PLAYERS];

void init_board(void);
void reset_board(state_s *state);
void setup_board(state_s *, const piece_e *, player_e, castle_rights_t, bitboard_t, int, int);

bitboard_t get_attacks(state_s *state, square_e target, player_e attacking);
void make_move(state_s *state, move_s *move);
void change_player(state_s *state);
int check_legality(state_s *state, move_s *move);

/* Two moves are identical */
static inline int move_equal(move_s *move1, move_s *move2) {
  return (move1 && move2 && move1->from == move2->from && move1->to == move2->to &&
          move1->promotion == move2->promotion);
}

/* Square coordinate is valid, rejecting NO_SQUARE */
static inline int is_valid_square(square_e square) { return (square >= 0 && square < N_SQUARES); }
/* Convert bitboard bit to square coordinate */
static inline square_e bit2square(bitboard_t mask) {
  ASSERT(is_valid_square((square_e)ctz(mask)));
  return (square_e)ctz(mask);
}
/* Clear the position to an empty board */
static inline void clear_state(state_s *state) { memset(state, 0, sizeof(state_s)); }
/* memcpy the position */
static inline void copy_state(state_s *dst, const state_s *src) {
  memcpy(dst, src, sizeof(state_s));
}
/* Return the set of squares that the piece on the given square can move to */
static inline bitboard_t get_moves(state_s *state, square_e square) {
  return state->moves[(int)state->index_at[square]];
}
/* Return the set of squares containing the moving player's pieces */
static inline bitboard_t get_my_pieces(state_s *state) { return state->player_a[state->turn]; }
/* Return the set of squares containing the non-moving player's pieces */
static inline bitboard_t get_opponents_pieces(state_s *state) {
  return state->player_a[state->turn];
}
/* The player to move is in check */
static inline int in_check(state_s *state) { return state->check[state->turn]; }
/* The move goes to the back row, true even if not a pawn. */
static inline int is_promotion_move(state_s *state, square_e from, square_e to) {
  return ((square2bit[to] & 0xff000000000000ffull) != 0);
}
/* There is no piece at the given square */
static inline int no_piece_at_square(state_s *state, square_e square) {
  return ((square2bit[square] & state->total_a) == 0);
}

#endif /* STATE_H */
