/*
 *  Game state
 */

#include "moves.h"
#include "state.h"
#include "log.h"

void init_moves(void);

/* 
 *  Lookup tables
 */

/* Mapping from A-pos to B-pos
   Calculated at init using a formula */
pos_t pos_a2b[N_POS];

/* Mapping from A-pos to C-pos */
const pos_t pos_a2c[N_POS] = {
  0,  1,  3,  6, 10, 15, 21, 28,
  2,  4,  7, 11, 16, 22, 29, 36,
  5,  8, 12, 17, 23, 30, 37, 43,
  9, 13, 18, 24, 31, 38, 44, 49,
  14, 19, 25, 32, 39, 45, 50, 54,
  20, 26, 33, 40, 46, 51, 55, 58,
  27, 34, 41, 47, 52, 56, 59, 61,
  35, 42, 48, 53, 57, 60, 62, 63
};
pos_t pos_a2d[N_POS];

/* Castling */
const pos_t rook_start_pos[N_PLAYERS][2] = { { 0, 7 }, { 56, 63 } };
const pos_t king_start_pos[N_PLAYERS] = { 4, 60 };
const plane_t castle_moves[N_PLAYERS][2] = { { 0x01ull, 0x80ull }, { 0x01ull << 56, 0x80ull << 56 } };

const castle_rights_t castling_rights[N_PLAYERS][N_BOARDSIDE] = {
  { WHITE_QUEENSIDE, WHITE_KINGSIDE, WHITE_BOTHSIDES },
  { BLACK_QUEENSIDE, BLACK_KINGSIDE, BLACK_BOTHSIDES }
};

const char start_indexes[N_POS] = {
   0,   1,   2,   3,   4,   5,   6,   7,
   8,   9,  10,  11,  12,  13,  14,  15,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,
  16,  17,  18,  19,  20,  21,  22,  23, 
  24,  25,  26,  27,  28,  29,  30,  31
};
const char king_index[N_PLAYERS] = {
  4, 28
};
const piece_e piece_type[N_PIECE_T * N_PLAYERS] = {
  PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING,
  PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING  
};

const piece_e start_pieces[N_POS] = { 
  ROOK,  KNIGHT, BISHOP, QUEEN, KING,  BISHOP, KNIGHT, ROOK,
  PAWN,  PAWN,   PAWN,   PAWN,  PAWN,  PAWN,   PAWN,   PAWN,
  EMPTY, EMPTY,  EMPTY,  EMPTY, EMPTY, EMPTY,  EMPTY,  EMPTY,
  EMPTY, EMPTY,  EMPTY,  EMPTY, EMPTY, EMPTY,  EMPTY,  EMPTY,
  EMPTY, EMPTY,  EMPTY,  EMPTY, EMPTY, EMPTY,  EMPTY,  EMPTY,
  EMPTY, EMPTY,  EMPTY,  EMPTY, EMPTY, EMPTY,  EMPTY,  EMPTY,
  PAWN+N_PIECE_T,  PAWN+N_PIECE_T,   PAWN+N_PIECE_T,   PAWN+N_PIECE_T,  PAWN+N_PIECE_T,  PAWN+N_PIECE_T,   PAWN+N_PIECE_T,   PAWN+N_PIECE_T,
  ROOK+N_PIECE_T,  KNIGHT+N_PIECE_T, BISHOP+N_PIECE_T, QUEEN+N_PIECE_T, KING+N_PIECE_T,  BISHOP+N_PIECE_T, KNIGHT+N_PIECE_T, ROOK+N_PIECE_T 
};

const player_e piece_player[N_PIECE_T * N_PLAYERS] = {
  WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
};

const player_e opponent[N_PLAYERS] = { BLACK, WHITE };
plane_t pos2mask[N_POS];


/*
 *  Functions 
 */

static inline void add_piece(state_s *state, pos_t pos, piece_e piece, int index)
{
  ASSERT(piece != EMPTY);
  ASSERT(index != EMPTY);
  ASSERT(state->piece_at[pos] == EMPTY);
  piece_e player = piece_player[piece];
  plane_t a_mask = pos2mask[pos];
  plane_t b_mask = pos2mask[pos_a2b[pos]];
  plane_t c_mask = pos2mask[pos_a2c[pos]];
  plane_t d_mask = pos2mask[pos_a2d[pos]];
  state->a[piece] |= a_mask;
  state->b[piece] |= b_mask;
  state->c[piece] |= c_mask;
  state->d[piece] |= d_mask;
  state->occ_a[player] |= a_mask;
  state->occ_b[player] |= b_mask;
  state->occ_c[player] |= c_mask;
  state->occ_d[player] |= d_mask;
  state->total_a |= a_mask;
  state->total_b |= b_mask;
  state->total_c |= c_mask;
  state->total_d |= d_mask;
  state->piece_pos[(int)index] = pos;
  state->piece_at[pos] = piece;
  state->index_at[pos] = index;
}

static inline void remove_piece(state_s *state, pos_t pos)
{
  ASSERT(state->piece_at[pos] != EMPTY);
  int8_t piece = state->piece_at[pos];
  piece_e player = piece_player[piece];
  plane_t a_mask = pos2mask[pos];
  plane_t b_mask = pos2mask[pos_a2b[pos]];
  plane_t c_mask = pos2mask[pos_a2c[pos]];
  plane_t d_mask = pos2mask[pos_a2d[pos]];
  state->a[piece] &= ~a_mask;
  state->b[piece] &= ~b_mask;
  state->c[piece] &= ~c_mask;
  state->d[piece] &= ~d_mask;
  state->occ_a[player] &= ~a_mask;
  state->occ_b[player] &= ~b_mask;
  state->occ_c[player] &= ~c_mask;
  state->occ_d[player] &= ~d_mask;
  state->total_a &= ~a_mask;
  state->total_b &= ~b_mask;
  state->total_c &= ~c_mask;
  state->total_d &= ~d_mask;
  state->piece_pos[(int)state->index_at[pos]] = NO_POS;
  state->piece_at[pos] = EMPTY;
  state->index_at[pos] = EMPTY;
}

static inline void clear_rook_castling_rights(state_s *state, pos_t pos, player_e player)
{
  for(int side = 0; side < 2; side++) {
    if(pos == rook_start_pos[player][side]) {
      state->castling_rights &= ~castling_rights[player][side];
    }
  }
}

static inline void clear_king_castling_rights(state_s *state, player_e player)
{
  state->castling_rights &= ~castling_rights[player][BOTHSIDES];
}

static inline void do_rook_castling_move(state_s *state, 
  pos_t king_pos, pos_t from_offset, pos_t to_offset)
{
  uint8_t rook_piece = state->piece_at[king_pos + from_offset];
  int8_t rook_index = state->index_at[king_pos + from_offset];
  remove_piece(state, king_pos + from_offset);
  add_piece(state, king_pos + to_offset, rook_piece, rook_index);
}

/* Alters the game state to effect a move.  There is no validity checking. */
void make_move(state_s *state, move_s *move)
{
  ASSERT(is_valid_pos(move->from));
  ASSERT(is_valid_pos(move->to));
  ASSERT(move->from != move->to);

  move->result = 0;

  /* Taking */
  int8_t victim_piece = state->piece_at[move->to];
  if(victim_piece != EMPTY) {
    ASSERT(piece_type[victim_piece] != KING);
    player_e victim_player = piece_player[victim_piece];
    ASSERT(victim_player != state->to_move);
    remove_piece(state, move->to);
    move->result |= CAPTURED;
    /* If a rook has been captured, treat it as moved to prevent castling */
    if(piece_type[victim_piece] == ROOK) {
      clear_rook_castling_rights(state, move->to, victim_player);
    }
  }

  /* Moving */
  uint8_t moving_piece = state->piece_at[move->from];
  ASSERT(piece_player[moving_piece] == state->to_move);
  int8_t moving_index = state->index_at[move->from];
  remove_piece(state, move->from);
  add_piece(state, move->to, moving_piece, moving_index);

  /* Castling, pawn promotion and other special stuff */
  switch(piece_type[moving_piece]) {
  case KING:
    /* If this is a castling move (2 spaces each direction) */
    if(move->from == move->to + 2) {
      do_rook_castling_move(state, move->to, -2, +1);
      move->result |= CASTLED;
    } else if(move->from == move->to - 2) {
      do_rook_castling_move(state, move->to, +1, -1);
      move->result |= CASTLED;
    }
    clear_king_castling_rights(state, piece_player[moving_piece]);
    break;
  case ROOK:
    clear_rook_castling_rights(state, move->from, state->to_move);
    break;
  case PAWN:
    if(is_promotion_move(state, move->from, move->to)) {
      ASSERT(move->promotion > PAWN);
      remove_piece(state, move->to);
      add_piece(state, move->to, moving_piece + move->promotion - PAWN,
        moving_index);
      move->result |= PROMOTED;
    }
    /* If pawn has been taken en-passant */
    if(move->to == state->en_passant) {
      pos_t target_pos = move->to;
      if(state->to_move == WHITE) target_pos -= 8;
      else target_pos += 8;
      ASSERT(state->piece_at[target_pos] != EMPTY);
      ASSERT(piece_player[state->piece_at[target_pos]] != state->to_move);
      remove_piece(state, target_pos);
      move->result |= EN_PASSANT | CAPTURED;
    }
    break;
  default:
    break;
  }
  /* Clear previous en passant state */
  state->en_passant = NO_POS;
  /* If pawn has jumped, set en passant square */
  if(piece_type[moving_piece] == PAWN) {
    if(move->from - move->to == 16) {
      state->en_passant = move->from - 8;
    }
    if(move->from - move->to == -16) {
      state->en_passant = move->from + 8;
    }
  }

  /* Calculate moves for all pieces */
  calculate_moves(state);
  
  /* Check testing */
  for(player_e player = 0; player < N_PLAYERS; player++) {
    int king_pos = mask2pos(state->a[KING + player * N_PIECE_T]);
    if(get_attacks(state, king_pos, opponent[player])) {
      state->check[player] = 1;
    } else {
      state->check[player] = 0;
    }
  }
  if(state->check[state->to_move]) {
    move->result |= CHECK;
  }
}

/* Uses infomration in pieces to generate the board state.
 * This is used by reset_board and load_fen */
void setup_board(state_s *state, const int *pieces, 
  player_e to_move, castle_rights_t castling_rights, pos_t en_passant)
{
  memset(state, 0, sizeof(*state));
  state->to_move = to_move;
  state->castling_rights = castling_rights;
  state->en_passant = en_passant;

  for(int i = 0; i < N_PIECES; i++) {
    state->piece_pos[i] = NO_POS;
  }
  for(pos_t pos = 0; pos < N_POS; pos++) {
    state->index_at[pos] = EMPTY;      
    state->piece_at[pos] = EMPTY;      
  }
  
  /* Iterate through positions */
  int index = 0;
  for(pos_t pos = 0; pos < N_POS; pos++) {
    int piece = pieces[pos];
    if(piece != EMPTY) {
      add_piece(state, pos, piece, index);
      index++;
    }
  }  
  /* Generate moves */
  calculate_moves(state);
}

/* Resets the board to the starting position */
void reset_board(state_s *state)
{
  setup_board(state, start_pieces, WHITE, ALL_CASTLE_RIGHTS, NO_POS);
}

/* Initialises the module */
void init_board(void)
{
  for(pos_t pos = 0; pos < N_POS; pos++) {
    pos2mask[pos] = 1ull << pos;
    pos_a2b[pos] = (pos / 8) + (pos % 8) * 8;
    pos_a2d[pos] = pos_a2c[(pos / 8) * 8 + (7 - pos % 8)];
  }
  init_moves();
}