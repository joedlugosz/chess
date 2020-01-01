#ifndef BOARD_H
#define BOARD_H

#include "chess.h"
#include "lowlevel.h"
//#include "hash.h"

typedef int pos_t;
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
  /* Squares on the board */
  N_SQUARES = 64,
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
  pos_t pos[N_PIECES];       /* Board position of each piece */
  char piece_at[N_SQUARES];  /* Piece index at board position */
  char index_at[N_SQUARES];  /* Piece index at board position */
  /* Other info */
  pos_t from;                /* Position moved from to get to this state */
  pos_t to;                  /* Ditto */
  status_t captured : 1;     /* Whether the last move captured */
  status_t castled : 1;      /* Whether the last move castled */
  status_t to_move : 1;      /* Player to move next */
  status_t check[N_PLAYERS]; /* Whether each player is in check */
  plane_t moved;             /* Flags for pieces which have moved */
  hash_t hash;
} state_s;

void init_board(void);
void reset_board(state_s *state);
void setup_board(state_s *state, const int *pieces, player_e to_move, plane_t pieces_moved);
//void random_state(state_s *s);
plane_t get_attacks(state_s *state, pos_t target, player_e attacking);
void do_move(state_s *state, pos_t from, pos_t to);

extern plane_t pos2mask[N_SQUARES];
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
  return (pos >= 0 && pos < N_SQUARES);
}
#endif /* BOARD_H */
