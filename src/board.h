/*
 *  board.h
 *
 *  Definition of the game state and FIDE rules.
 *
 *  Limitations:
 *   * No under-promotion
 *
 *  Revisions:
 *   5.1            En passant, underpromotion
 */

#ifndef BOARD_H
#define BOARD_H

#include "chess.h"
#include "lowlevel.h"
#include "board.h"
#include <stdint.h>
//typedef int pos_t;
typedef enum {
  NO_POS = -1,
  A1 = 0,
      B1, C1, D1, E1, F1, G1, H1,
  A2, B2, C2, D2, E2, F2, G2, H2,
  A3, B3, C3, D3, E3, F3, G3, H3,
  A4, B4, C4, D4, E4, F4, G4, H4,
  A5, B5, C5, D5, E5, F5, G5, H5,
  A6, B6, C6, D6, E6, F6, G6, H6,
  A7, B7, C7, D7, E7, F7, G7, H7,
  A8, B8, C8, D8, E8, F8, G8, H8,
  N_POS
} pos_t;

typedef unsigned char status_t;

/* Piece types */
typedef enum piece_e_ {
  EMPTY = -1,
  PAWN = 0,
  ROOK, KNIGHT, BISHOP, QUEEN, KING,
  N_PIECE_T
} piece_e;

enum {
  /* Planes in a stack */
  N_PLANES = N_PIECE_T * N_PLAYERS,
  /* Pieces */
  N_PIECES = 32,
};

/* Four stacks are used to represent the same information with different bit 
   orders, with the squares indexed according to horizontal, vertical and 
   diagonal schemes. */
typedef struct state_s_ {
  /* The stacks */
  plane_t a[N_PLANES];       /* -  Horizontal    */
  plane_t b[N_PLANES];       /* |  Vertical      */
  plane_t c[N_PLANES];       /* /  Bend Dexter   */
  plane_t d[N_PLANES];       /* \  Bend Sinister */
  plane_t occ_a[N_PLAYERS];  /* Set of each players pieces */
  plane_t occ_b[N_PLAYERS];  /* Set of each players pieces */
  plane_t occ_c[N_PLAYERS];  /* Set of each players pieces */
  plane_t occ_d[N_PLAYERS];  /* Set of each players pieces */
  plane_t total_a;           /* Total occupancy */
  plane_t total_b;           /* Total occupancy */
  plane_t total_c;           /* Total occupancy */
  plane_t total_d;           /* Total occupancy */
  plane_t moves[N_PIECES];   /* Set of squares each piece can move to */
  plane_t claim[N_PLAYERS];  /* Set of all squares each player can move to */
  pos_t piece_pos[N_PIECES];       /* Board position of each piece */
  int8_t piece_at[N_POS];  /* Piece index at board position */
  int8_t index_at[N_POS];  /* Piece index at board position */

  /* Other info */
  pos_t from;                /* Position moved from to get to this state */
  pos_t to;                  /* Ditto */
  status_t captured : 1;     /* Whether the last move captured */
  status_t ep_captured : 1;  /* Whether the last move capture was en passant */
  status_t promoted : 1;     /* Whether the last move promoted */
  status_t castled : 1;      /* Whether the last move castled */
  status_t to_move : 1;      /* Player to move next */
  status_t check[N_PLAYERS]; /* Whether each player is in check */
  plane_t moved;             /* Flags for pieces which have moved */
  plane_t en_passant;        /* En-passant squares */
  hash_t hash;
} state_s;

/* move_s holds the game state as well as info about moves */
typedef struct move_s_ {
  pos_t from, to;
  piece_e promotion;
} move_s;

void init_board(void);
void reset_board(state_s *state);
void setup_board(state_s *state, const int *pieces, player_e to_move, plane_t pieces_moved);
//void random_state(state_s *s);
plane_t get_attacks(state_s *state, pos_t target, player_e attacking);
void make_move(state_s *state, move_s *move);

extern plane_t pos2mask[N_POS];
extern const piece_e piece_type[N_PLANES];
extern const player_e piece_player[N_PLANES];
extern const player_e opponent[N_PLAYERS];

static inline pos_t mask2pos(plane_t mask) {
  return (pos_t)ctz(mask);
}
static inline void clear_state(state_s *state) {
  memset(state, 0, sizeof(state_s));
}
static inline void copy_state(state_s *dst, const state_s *src) {
  memcpy(dst, src, sizeof(state_s));
}
static inline plane_t get_moves(state_s *state, pos_t pos) {
  return state->moves[(int)state->index_at[pos]];
}
static inline plane_t get_my_pieces(state_s *state) {
  return state->occ_a[state->to_move];
}
static inline int in_check(state_s *state) {
  return state->check[state->to_move];
}
static inline void change_player(state_s *state) {
  state->to_move = opponent[state->to_move];
}
static inline int is_valid_pos(pos_t pos) {
  return (pos >= 0 && pos < N_POS);
}
static inline int is_promotion_move(state_s *state, pos_t from, plane_t to_mask) {
  if((to_mask & 0xff000000000000ffull) 
    && piece_type[(int)state->piece_at[from]] == PAWN) {
      return 1;
  }
  return 0;
}
#endif /* BOARD_H */
