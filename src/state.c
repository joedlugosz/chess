/*
 *  Game state
 */

#include "state.h"

#include "debug.h"
#include "fen.h"
#include "hash.h"
#include "moves.h"

void init_moves(void);

/*
 *  Lookup tables
 */

/* clang-format off */

/* Mapping from A-square to B-square
   Calculated at init using a formula */
square_e square_a2b[N_SQUARES];

/* Mapping from A-square to C-square */
const square_e square_a2c[N_SQUARES] = {
  0,  1,  3,  6, 10, 15, 21, 28,
  2,  4,  7, 11, 16, 22, 29, 36,
  5,  8, 12, 17, 23, 30, 37, 43,
  9, 13, 18, 24, 31, 38, 44, 49,
  14, 19, 25, 32, 39, 45, 50, 54,
  20, 26, 33, 40, 46, 51, 55, 58,
  27, 34, 41, 47, 52, 56, 59, 61,
  35, 42, 48, 53, 57, 60, 62, 63
};
square_e square_a2d[N_SQUARES];

/* Castling */
const square_e rook_start_square[N_PLAYERS][2] = { { 0, 7 }, { 56, 63 } };
const square_e king_start_square[N_PLAYERS] = { 4, 60 };
const bitboard_t castle_moves[N_PLAYERS][2] = { { 0x01ull, 0x80ull }, { 0x01ull << 56, 0x80ull << 56 } };

const castle_rights_t castling_rights[N_PLAYERS][N_BOARDSIDE] = {
  { WHITE_QUEENSIDE, WHITE_KINGSIDE, WHITE_BOTHSIDES },
  { BLACK_QUEENSIDE, BLACK_KINGSIDE, BLACK_BOTHSIDES }
};

const char start_indexes[N_SQUARES] = {
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

const piece_e start_pieces[N_SQUARES] = { 
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

/* clang-format on */

const player_e opponent[N_PLAYERS] = {BLACK, WHITE};
bitboard_t _square2bit[N_SQUARES + 1];
bitboard_t *square2bit;

/*
 *  Functions
 */

static inline void add_piece(state_s *state, square_e square, piece_e piece, int index) {
  ASSERT(piece != EMPTY);
  ASSERT(index != EMPTY);
  ASSERT(state->piece_at[square] == EMPTY);
  piece_e player = piece_player[piece];
  bitboard_t a_mask = square2bit[square];
  bitboard_t b_mask = square2bit[square_a2b[square]];
  bitboard_t c_mask = square2bit[square_a2c[square]];
  bitboard_t d_mask = square2bit[square_a2d[square]];
  state->a[piece] |= a_mask;
  state->b[piece] |= b_mask;
  state->c[piece] |= c_mask;
  state->d[piece] |= d_mask;
  state->player_a[player] |= a_mask;
  state->player_b[player] |= b_mask;
  state->player_c[player] |= c_mask;
  state->player_d[player] |= d_mask;
  state->total_a |= a_mask;
  state->total_b |= b_mask;
  state->total_c |= c_mask;
  state->total_d |= d_mask;
  state->piece_square[(int)index] = square;
  state->piece_at[square] = piece;
  state->index_at[square] = index;
  state->hash ^= placement_key[piece][square];
}

static inline void remove_piece(state_s *state, square_e square) {
  ASSERT(state->piece_at[square] != EMPTY);
  int8_t piece = state->piece_at[square];
  piece_e player = piece_player[piece];
  bitboard_t a_mask = square2bit[square];
  bitboard_t b_mask = square2bit[square_a2b[square]];
  bitboard_t c_mask = square2bit[square_a2c[square]];
  bitboard_t d_mask = square2bit[square_a2d[square]];
  state->a[piece] &= ~a_mask;
  state->b[piece] &= ~b_mask;
  state->c[piece] &= ~c_mask;
  state->d[piece] &= ~d_mask;
  state->player_a[player] &= ~a_mask;
  state->player_b[player] &= ~b_mask;
  state->player_c[player] &= ~c_mask;
  state->player_d[player] &= ~d_mask;
  state->total_a &= ~a_mask;
  state->total_b &= ~b_mask;
  state->total_c &= ~c_mask;
  state->total_d &= ~d_mask;
  state->piece_square[(int)state->index_at[square]] = NO_SQUARE;
  state->piece_at[square] = EMPTY;
  state->index_at[square] = EMPTY;
  state->hash ^= placement_key[piece][square];
}

static inline void clear_rook_castling_rights(state_s *state, square_e square, player_e player) {
  for (boardside_e side = QUEENSIDE; side <= KINGSIDE; side++) {
    if (square == rook_start_square[player][side]) {
      if (state->castling_rights & castling_rights[player][side]) {
        state->hash ^= castle_rights_key[player][side];
        state->castling_rights &= ~castling_rights[player][side];
      }
    }
  }
}

static inline void clear_king_castling_rights(state_s *state, player_e player) {
  for (boardside_e side = QUEENSIDE; side <= KINGSIDE; side++) {
    if (state->castling_rights & castling_rights[player][side]) {
      state->hash ^= castle_rights_key[player][side];
      state->castling_rights &= ~castling_rights[player][side];
    }
  }
}

static inline void do_rook_castling_move(state_s *state, square_e king_square, square_e from_offset,
                                         square_e to_offset) {
  uint8_t rook_piece = state->piece_at[king_square + from_offset];
  int8_t rook_index = state->index_at[king_square + from_offset];
  remove_piece(state, king_square + from_offset);
  add_piece(state, king_square + to_offset, rook_piece, rook_index);
}

/* Alters the game state to effect a move.  There is no validity checking. */
void make_move(state_s *state, move_s *move) {
  ASSERT(is_valid_square(move->from));
  ASSERT(is_valid_square(move->to));
  ASSERT(move->from != move->to);

  move->result = 0;

  /* Taking */
  int8_t victim_piece = state->piece_at[move->to];
  if (victim_piece != EMPTY) {
    ASSERT(piece_type[victim_piece] != KING);
    player_e victim_player = piece_player[victim_piece];
    ASSERT(victim_player != state->turn);
    remove_piece(state, move->to);
    move->result |= CAPTURED;
    /* If a rook has been captured, treat it as moved to prevent castling */
    if (piece_type[victim_piece] == ROOK) {
      clear_rook_castling_rights(state, move->to, victim_player);
    }
  }

  /* Moving */
  uint8_t moving_piece = state->piece_at[move->from];
  ASSERT(piece_player[moving_piece] == state->turn);
  int8_t moving_index = state->index_at[move->from];
  remove_piece(state, move->from);
  add_piece(state, move->to, moving_piece, moving_index);

  /* Castling, pawn promotion and other special stuff */
  switch (piece_type[moving_piece]) {
    case KING:
      /* If this is a castling move (2 spaces each direction) */
      if (move->from == move->to + 2) {
        do_rook_castling_move(state, move->to, -2, +1);
        move->result |= CASTLED;
      } else if (move->from == move->to - 2) {
        do_rook_castling_move(state, move->to, +1, -1);
        move->result |= CASTLED;
      }
      clear_king_castling_rights(state, piece_player[moving_piece]);
      break;
    case ROOK:
      clear_rook_castling_rights(state, move->from, state->turn);
      break;
    case PAWN:
      if (is_promotion_move(state, move->from, move->to)) {
        ASSERT(move->promotion > PAWN);
        remove_piece(state, move->to);
        add_piece(state, move->to, moving_piece + move->promotion - PAWN, moving_index);
        move->result |= PROMOTED;
      }
      /* If pawn has been taken en-passant */
      if (square2bit[move->to] == state->en_passant) {
        square_e target_square = move->to;
        if (state->turn == WHITE)
          target_square -= 8;
        else
          target_square += 8;
        ASSERT(state->piece_at[target_square] != EMPTY);
        ASSERT(piece_player[state->piece_at[target_square]] != state->turn);
        remove_piece(state, target_square);
        move->result |= EN_PASSANT | CAPTURED;
      }
      break;
    default:
      break;
  }
  /* Clear previous en passant state */
  state->en_passant = 0;
  /* If pawn has jumped, set en passant square */
  if (piece_type[moving_piece] == PAWN) {
    if (move->from - move->to == 16) {
      state->en_passant = square2bit[move->from - 8];
    }
    if (move->from - move->to == -16) {
      state->en_passant = square2bit[move->from + 8];
    }
  }

  /* Calculate moves for all pieces */
  calculate_moves(state);

  /* Check testing */
  for (player_e player = 0; player < N_PLAYERS; player++) {
    int king_square = bit2square(state->a[KING + player * N_PIECE_T]);
    if (get_attacks(state, king_square, opponent[player])) {
      state->check[player] = 1;
    } else {
      state->check[player] = 0;
    }
  }

  if (state->check[state->turn]) {
    move->result |= CHECK;
  }
}

int check_legality(state_s *state, move_s *move) {
  if (no_piece_at_square(state, move->from)) return ERR_NO_PIECE;
  if (move->from == move->to) return ERR_SRC_EQUAL_DEST;
  if ((square2bit[move->from] & get_my_pieces(state)) == 0) return ERR_NOT_MY_PIECE;
  if ((square2bit[move->to] & get_moves(state, move->from)) == 0) return ERR_CANT_MOVE_THERE;

  return 0;
}

/* Uses infomration in pieces to generate the board state.
 * This is used by reset_board and load_fen */
void setup_board(state_s *state, const piece_e *pieces, player_e turn,
                 castle_rights_t castling_rights, bitboard_t en_passant) {
  memset(state, 0, sizeof(*state));
  state->hash = init_key;
  state->turn = turn;
  state->castling_rights = castling_rights;
  state->en_passant = en_passant;

  memset(state->piece_square, NO_SQUARE, N_PIECES * sizeof(state->piece_square[0]));
  memset(state->index_at, EMPTY, N_SQUARES * sizeof(state->index_at[0]));
  memset(state->piece_at, EMPTY, N_SQUARES * sizeof(state->piece_at[0]));

  /* Iterate through positions */
  int index = 0;
  for (square_e square = 0; square < N_SQUARES; square++) {
    piece_e piece = pieces[square];
    if (piece != EMPTY) {
      add_piece(state, square, piece, index);
      index++;
    }
  }
  /* Generate moves */
  calculate_moves(state);
}

/* Resets the board to the starting position */
void reset_board(state_s *state) { setup_board(state, start_pieces, WHITE, ALL_CASTLE_RIGHTS, 0); }

/* Initialises the module */
void init_board(void) {
  square2bit = _square2bit + 1;
  square2bit[NO_SQUARE] = 0;
  for (square_e square = 0; square < N_SQUARES; square++) {
    square2bit[square] = 1ull << square;
    square_a2b[square] = (square / 8) + (square % 8) * 8;
    square_a2d[square] = square_a2c[(square / 8) * 8 + (7 - square % 8)];
  }
  init_moves();
}
