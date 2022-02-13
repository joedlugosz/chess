/*
 *  Game state
 */

#ifndef STATE_H
#define STATE_H

#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "lowlevel.h"

/* Players */
typedef enum player_e_ { WHITE = 0, BLACK, N_PLAYERS } player_e;

extern const char player_text[N_PLAYERS][6];

typedef unsigned char status_t;

/* Squares on the board */
typedef enum {
  NO_SQUARE = -1,
  A1 = 0,
  B1,
  C1,
  D1,
  E1,
  F1,
  G1,
  H1,
  A2,
  B2,
  C2,
  D2,
  E2,
  F2,
  G2,
  H2,
  A3,
  B3,
  C3,
  D3,
  E3,
  F3,
  G3,
  H3,
  A4,
  B4,
  C4,
  D4,
  E4,
  F4,
  G4,
  H4,
  A5,
  B5,
  C5,
  D5,
  E5,
  F5,
  G5,
  H5,
  A6,
  B6,
  C6,
  D6,
  E6,
  F6,
  G6,
  H6,
  A7,
  B7,
  C7,
  D7,
  E7,
  F7,
  G7,
  H7,
  A8,
  B8,
  C8,
  D8,
  E8,
  F8,
  G8,
  H8,
  N_SQUARES
} square_e;

/* Sides of the board */
typedef enum boardside_e { QUEENSIDE = 0, KINGSIDE, BOTHSIDES, N_BOARDSIDE } boardside_e;

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

/* Piece types */
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

/* Pieces */
enum {
  N_PIECES = 32,
};

/* Bitboard - a 64-bit number describing a set of squares on the board */
typedef unsigned long long bitboard_t;

/* The state of the game */

/* Stacks of bitboards - there is one bitboard for each type of piece for each player */
enum {
  N_PLANES = N_PIECE_T * N_PLAYERS,
};

/* Four stacks are used to represent the same information with different bit
   orders, with the squares indexed according to horizontal, vertical and
   diagonal schemes. */
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
  square_e piece_square[N_PIECES]; /* Board position of each piece */
  int8_t piece_at[N_SQUARES];      /* Piece type at board position */
  int8_t index_at[N_SQUARES];      /* Piece index at board position */

  status_t turn : 1;         /* Player to move next */
  status_t check[N_PLAYERS]; /* Whether each player is in check */
  castle_rights_t castling_rights;
  bitboard_t en_passant; /* En-passant squares */
} state_s;

/* move_s holds the game state as well as info about moves */
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
typedef struct move_s_ {
  square_e from, to;
  piece_e promotion;
  moveresult_t result;
} move_s;

enum { ERR_BASE = 0, ERR_NO_PIECE, ERR_SRC_EQUAL_DEST, ERR_NOT_MY_PIECE, ERR_CANT_MOVE_THERE };

extern bitboard_t *square2bit;
extern const piece_e piece_type[N_PLANES];
extern const player_e piece_player[N_PLANES];
extern const player_e opponent[N_PLAYERS];

void init_board(void);
void reset_board(state_s *state);
void setup_board(state_s *, const piece_e *, player_e, castle_rights_t, bitboard_t);

bitboard_t get_attacks(state_s *state, square_e target, player_e attacking);
void make_move(state_s *state, move_s *move);
static inline void change_player(state_s *state) { state->turn = opponent[state->turn]; }
int check_legality(state_s *state, move_s *move);
static inline int move_equal(move_s *move1, move_s *move2) {
  return (move1 && move2 && move1->from == move2->from && move1->to == move2->to &&
          move1->promotion == move2->promotion);
}

static inline int is_valid_square(square_e square) { return (square >= 0 && square < N_SQUARES); }
static inline square_e bit2square(bitboard_t mask) {
  ASSERT(is_valid_square((square_e)ctz(mask)));
  return (square_e)ctz(mask);
}
static inline void clear_state(state_s *state) { memset(state, 0, sizeof(state_s)); }
static inline void copy_state(state_s *dst, const state_s *src) {
  memcpy(dst, src, sizeof(state_s));
}
static inline bitboard_t get_moves(state_s *state, square_e square) {
  return state->moves[(int)state->index_at[square]];
}
static inline bitboard_t get_my_pieces(state_s *state) { return state->player_a[state->turn]; }
static inline bitboard_t get_opponents_pieces(state_s *state) {
  return state->player_a[state->turn];
}
static inline int in_check(state_s *state) { return state->check[state->turn]; }
static inline int is_promotion_move(state_s *state, square_e from, square_e to) {
  return ((square2bit[to] & 0xff000000000000ffull) != 0);
}
static inline int no_piece_at_square(state_s *state, square_e square) {
  return ((square2bit[square] & state->total_a) == 0);
}

#endif /* STATE_H */
