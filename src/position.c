/*
 *  Game position
 */

#include "position.h"

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
enum square square_a2b[N_SQUARES];

/* Mapping from A-square to C-square */
const enum square square_a2c[N_SQUARES] = {
   0,  1,  3,  6, 10, 15, 21, 28,
   2,  4,  7, 11, 16, 22, 29, 36,
   5,  8, 12, 17, 23, 30, 37, 43,
   9, 13, 18, 24, 31, 38, 44, 49,
  14, 19, 25, 32, 39, 45, 50, 54,
  20, 26, 33, 40, 46, 51, 55, 58,
  27, 34, 41, 47, 52, 56, 59, 61,
  35, 42, 48, 53, 57, 60, 62, 63
};
enum square square_a2d[N_SQUARES];

/* Castling */
const enum square rook_start_square[N_PLAYERS][2] = { { 0, 7 }, { 56, 63 } };
const enum square king_start_square[N_PLAYERS] = { 4, 60 };
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
const enum piece piece_type[N_PIECE_T * N_PLAYERS] = {
  PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING,
  PAWN, ROOK, KNIGHT, BISHOP, QUEEN, KING  
};

const enum piece start_pieces[N_SQUARES] = { 
  ROOK,  KNIGHT, BISHOP, QUEEN, KING,  BISHOP, KNIGHT, ROOK,
  PAWN,  PAWN,   PAWN,   PAWN,  PAWN,  PAWN,   PAWN,   PAWN,
  EMPTY, EMPTY,  EMPTY,  EMPTY, EMPTY, EMPTY,  EMPTY,  EMPTY,
  EMPTY, EMPTY,  EMPTY,  EMPTY, EMPTY, EMPTY,  EMPTY,  EMPTY,
  EMPTY, EMPTY,  EMPTY,  EMPTY, EMPTY, EMPTY,  EMPTY,  EMPTY,
  EMPTY, EMPTY,  EMPTY,  EMPTY, EMPTY, EMPTY,  EMPTY,  EMPTY,
  PAWN+N_PIECE_T,  PAWN+N_PIECE_T,   PAWN+N_PIECE_T,   PAWN+N_PIECE_T,  PAWN+N_PIECE_T,  PAWN+N_PIECE_T,   PAWN+N_PIECE_T,   PAWN+N_PIECE_T,
  ROOK+N_PIECE_T,  KNIGHT+N_PIECE_T, BISHOP+N_PIECE_T, QUEEN+N_PIECE_T, KING+N_PIECE_T,  BISHOP+N_PIECE_T, KNIGHT+N_PIECE_T, ROOK+N_PIECE_T 
};

const enum player piece_player[N_PIECE_T * N_PLAYERS] = {
  WHITE, WHITE, WHITE, WHITE, WHITE, WHITE,
  BLACK, BLACK, BLACK, BLACK, BLACK, BLACK
};

/* clang-format on */

const enum player opponent[N_PLAYERS] = {BLACK, WHITE};
bitboard_t _square2bit[N_SQUARES + 1];
/* Convert square coordinate to bitboard bit */
bitboard_t *square2bit;

/*
 *  Functions
 */

static inline void add_piece(struct position *position, enum square square,
                             enum piece piece, int index) {
  ASSERT(piece != EMPTY);
  ASSERT(index != EMPTY);
  ASSERT(position->piece_at[square] == EMPTY);
  enum piece player = piece_player[piece];
  bitboard_t a_mask = square2bit[square];
  bitboard_t b_mask = square2bit[square_a2b[square]];
  bitboard_t c_mask = square2bit[square_a2c[square]];
  bitboard_t d_mask = square2bit[square_a2d[square]];
  position->a[piece] |= a_mask;
  position->b[piece] |= b_mask;
  position->c[piece] |= c_mask;
  position->d[piece] |= d_mask;
  position->player_a[player] |= a_mask;
  position->player_b[player] |= b_mask;
  position->player_c[player] |= c_mask;
  position->player_d[player] |= d_mask;
  position->total_a |= a_mask;
  position->total_b |= b_mask;
  position->total_c |= c_mask;
  position->total_d |= d_mask;
  position->piece_square[(int)index] = square;
  position->piece_at[square] = piece;
  position->index_at[square] = index;
  position->hash ^= placement_key[piece][square];
}

static inline void remove_piece(struct position *position, enum square square) {
  ASSERT(position->piece_at[square] != EMPTY);
  int8_t piece = position->piece_at[square];
  enum piece player = piece_player[piece];
  bitboard_t a_mask = square2bit[square];
  bitboard_t b_mask = square2bit[square_a2b[square]];
  bitboard_t c_mask = square2bit[square_a2c[square]];
  bitboard_t d_mask = square2bit[square_a2d[square]];
  position->a[piece] &= ~a_mask;
  position->b[piece] &= ~b_mask;
  position->c[piece] &= ~c_mask;
  position->d[piece] &= ~d_mask;
  position->player_a[player] &= ~a_mask;
  position->player_b[player] &= ~b_mask;
  position->player_c[player] &= ~c_mask;
  position->player_d[player] &= ~d_mask;
  position->total_a &= ~a_mask;
  position->total_b &= ~b_mask;
  position->total_c &= ~c_mask;
  position->total_d &= ~d_mask;
  position->piece_square[(int)position->index_at[square]] = NO_SQUARE;
  position->piece_at[square] = EMPTY;
  position->index_at[square] = EMPTY;
  position->hash ^= placement_key[piece][square];
}

static inline void clear_rook_castling_rights(struct position *position,
                                              enum square square,
                                              enum player player) {
  for (enum boardside side = QUEENSIDE; side <= KINGSIDE; side++) {
    if (square == rook_start_square[player][side]) {
      if (position->castling_rights & castling_rights[player][side]) {
        position->hash ^= castle_rights_key[player][side];
        position->castling_rights &= ~castling_rights[player][side];
      }
    }
  }
}

static inline void clear_king_castling_rights(struct position *position,
                                              enum player player) {
  for (enum boardside side = QUEENSIDE; side <= KINGSIDE; side++) {
    if (position->castling_rights & castling_rights[player][side]) {
      position->hash ^= castle_rights_key[player][side];
      position->castling_rights &= ~castling_rights[player][side];
    }
  }
}

static inline void do_rook_castling_move(struct position *position,
                                         enum square king_square,
                                         enum square from_offset,
                                         enum square to_offset) {
  uint8_t rook_piece = position->piece_at[king_square + from_offset];
  int8_t rook_index = position->index_at[king_square + from_offset];
  remove_piece(position, king_square + from_offset);
  add_piece(position, king_square + to_offset, rook_piece, rook_index);
}

/* Alter the position to make a move, without validity checking. Update `move`
   with the result */
void make_move(struct position *position, struct move *move) {
  ASSERT(is_valid_square(move->from));
  ASSERT(is_valid_square(move->to));
  ASSERT(move->from != move->to);

  move->result = 0;

  position->halfmove++;

  /* Taking */
  int8_t victim_piece = position->piece_at[move->to];
  if (victim_piece != EMPTY) {
    ASSERT(piece_type[victim_piece] != KING);
    enum player victim_player = piece_player[victim_piece];
    ASSERT(victim_player != position->turn);
    remove_piece(position, move->to);
    move->result |= CAPTURED;
    /* If a rook has been captured, treat it as moved to prevent castling */
    if (piece_type[victim_piece] == ROOK) {
      clear_rook_castling_rights(position, move->to, victim_player);
    }
    position->halfmove = 0;
  }

  /* Moving */
  uint8_t moving_piece = position->piece_at[move->from];
  ASSERT(piece_player[moving_piece] == position->turn);
  int8_t moving_index = position->index_at[move->from];
  remove_piece(position, move->from);
  add_piece(position, move->to, moving_piece, moving_index);

  /* Castling, pawn promotion and other special stuff */
  switch (piece_type[moving_piece]) {
    case KING:
      /* If this is a castling move (2 spaces each direction) */
      if (move->from == move->to + 2) {
        do_rook_castling_move(position, move->to, -2, +1);
        move->result |= CASTLED;
      } else if (move->from == move->to - 2) {
        do_rook_castling_move(position, move->to, +1, -1);
        move->result |= CASTLED;
      }
      clear_king_castling_rights(position, piece_player[moving_piece]);
      break;
    case ROOK:
      clear_rook_castling_rights(position, move->from, position->turn);
      break;
    case PAWN:
      if (is_promotion_move(position, move->from, move->to)) {
        ASSERT(move->promotion > PAWN);
        remove_piece(position, move->to);
        add_piece(position, move->to, moving_piece + move->promotion - PAWN,
                  moving_index);
        move->result |= PROMOTED;
      }
      /* If pawn has been taken en-passant */
      if (square2bit[move->to] == position->en_passant) {
        enum square target_square = move->to;
        if (position->turn == WHITE)
          target_square -= 8;
        else
          target_square += 8;
        ASSERT(position->piece_at[target_square] != EMPTY);
        ASSERT(piece_player[position->piece_at[target_square]] !=
               position->turn);
        remove_piece(position, target_square);
        move->result |= EN_PASSANT | CAPTURED;
      }
      position->halfmove = 0;
      break;
    default:
      break;
  }
  /* Clear previous en passant position */
  position->en_passant = 0;
  /* If pawn has jumped, set en passant square */
  if (piece_type[moving_piece] == PAWN) {
    if (move->from - move->to == 16) {
      position->en_passant = square2bit[move->from - 8];
    }
    if (move->from - move->to == -16) {
      position->en_passant = square2bit[move->from + 8];
    }
  }

  /* Calculate moves for all pieces */
  calculate_moves(position);

  /* Check testing */
  for (enum player player = 0; player < N_PLAYERS; player++) {
    int king_square = bit2square(position->a[KING + player * N_PIECE_T]);
    if (get_attacks(position, king_square, opponent[player])) {
      position->check[player] = 1;
    } else {
      position->check[player] = 0;
    }
  }

  position->ply++;
  if (position->turn == BLACK) position->fullmove++;
}

void change_player(struct position *position) {
  position->turn = opponent[position->turn];
  position->hash ^= turn_key;
}

int check_legality(const struct position *position, const struct move *move) {
  if (no_piece_at_square(position, move->from)) return ERR_NO_PIECE;
  if (move->from == move->to) return ERR_SRC_EQUAL_DEST;
  if ((square2bit[move->from] & get_my_pieces(position)) == 0)
    return ERR_NOT_MY_PIECE;
  if ((square2bit[move->to] & get_moves(position, move->from)) == 0)
    return ERR_CANT_MOVE_THERE;
  if ((is_promotion_move(position, move->from, move->to) &&
       (position->piece_at[move->from] == PAWN ||
        position->piece_at[move->from] == PAWN + N_PIECE_T)) ^
      (move->promotion > PAWN))
    return ERR_PROMOTION;
  return 0;
}

/* Uses infomration in pieces to generate the board position.
 * This is used by reset_board and load_fen */
void setup_board(struct position *position, const enum piece *pieces,
                 enum player turn, castle_rights_t castling_rights,
                 bitboard_t en_passant, int halfmove, int fullmove) {
  memset(position, 0, sizeof(*position));
  position->hash = init_key;
  position->turn = turn;
  position->castling_rights = castling_rights;
  position->en_passant = en_passant;
  position->halfmove = halfmove;
  position->fullmove = fullmove;

  memset(position->piece_square, NO_SQUARE,
         N_PIECES * sizeof(position->piece_square[0]));
  memset(position->index_at, EMPTY, N_SQUARES * sizeof(position->index_at[0]));
  memset(position->piece_at, EMPTY, N_SQUARES * sizeof(position->piece_at[0]));

  /* Iterate through positions */
  int index = 0;
  for (enum square square = 0; square < N_SQUARES; square++) {
    enum piece piece = pieces[square];
    if (piece != EMPTY) {
      add_piece(position, square, piece, index);
      index++;
    }
  }
  /* Generate moves */
  calculate_moves(position);
}

/* Resets the board to the starting position */
void reset_board(struct position *position) {
  setup_board(position, start_pieces, WHITE, ALL_CASTLE_RIGHTS, 0, 0, 1);
}

/* Initialises the module */
void init_board(void) {
  square2bit = _square2bit + 1;
  square2bit[NO_SQUARE] = 0;
  for (enum square square = 0; square < N_SQUARES; square++) {
    square2bit[square] = 1ull << square;
    square_a2b[square] = (square / 8) + (square % 8) * 8;
    square_a2d[square] = square_a2c[(square / 8) * 8 + (7 - square % 8)];
  }
  init_moves();
}
