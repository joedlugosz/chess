/*
 *  Piece moves and relationships
 */

#include "debug.h"
#include "io.h"
#include "position.h"

typedef unsigned char rank_t;
/* clang-format off */
/* For each A-square, the amount that the C-stack must be
   shifted to get the start of a diagonal row */
const enum square shift_c[N_SQUARES] = {
   0,  1,  3,  6, 10, 15, 21, 28,
   1,  3,  6, 10, 15, 21, 28, 36,
   3,  6, 10, 15, 21, 28, 36, 43,
   6, 10, 15, 21, 28, 36, 43, 49,
  10, 15, 21, 28, 36, 43, 49, 54,
  15, 21, 28, 36, 43, 49, 54, 58,
  21, 28, 36, 43, 49, 54, 58, 61,
  28, 36, 43, 49, 54, 58, 61, 63
};
/* For each A-square, the mask that must be applied to 
   obtain a single diagonal row */
const rank_t mask_c[N_SQUARES] = {
  0x01, 0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff,
  0x03, 0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f,
  0x07, 0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f,
  0x0f, 0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f,
  0x1f, 0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f,
  0x3f, 0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07,
  0x7f, 0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03,
  0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01
};
/* Amount that occupancy mask occ_c2a[occ] must 
   be shifted to get to the correct diagonal */
const enum square shift_c2a[N_SQUARES] = {
  0,  1,  2,  3,  4,  5,  6,  7,
  1,  2,  3,  4,  5,  6,  7, 15,
  2,  3,  4,  5,  6,  7, 15, 23,
  3,  4,  5,  6,  7, 15, 23, 31,
  4,  5,  6,  7, 15, 23, 31, 39,
  5,  6,  7, 15, 23, 31, 39, 47,
  6,  7, 15, 23, 31, 39, 47, 55,
  7, 15, 23, 31, 39, 47, 55, 63,
};
/* Ditto D-stack */
const enum square shift_d2a[N_SQUARES] = {
  0,  1,  2,  3,  4,  5,  6,  7,
  8,  0,  1,  2,  3,  4,  5,  6,
  16,  8,  0,  1,  2,  3,  4,  5,
  24, 16,  8,  0,  1,  2,  3,  4,
  32, 24, 16,  8,  0,  1,  2,  3,
  40, 32, 24, 16,  8,  0,  1,  2,
  48, 40, 32, 24, 16,  8,  0,  1,
  56, 48, 40, 32, 24, 16,  8,  0
};

/* Starting positions encoded as A-stack */
const bitboard_t starting_a[N_PLANES] = {
  /* White
     Black */
  /* Pawns     Rooks        Bishops      Knights      Queen        King       */
  0xffull<<8,  0x81ull,     0x42ull,     0x24ull,     0x08ull,     0x10ull,
  0xffull<<48, 0x81ull<<56, 0x42ull<<56, 0x24ull<<56, 0x08ull<<56, 0x10ull<<56
};
/* clang-format on */

const bitboard_t king_castle_slides[N_PLAYERS][2] = {
    {0x0cull, 0x60ull}, {0x0cull << 56, 0x60ull << 56}};
const bitboard_t rook_castle_slides[N_PLAYERS][2] = {
    {0x0eull, 0x60ull}, {0x0eull << 56, 0x60ull << 56}};
const bitboard_t castle_destinations[N_PLAYERS][2] = {
    {0x04ull, 0x40ull}, {0x04ull << 56, 0x40ull << 56}};

extern const enum square square_a2b[N_SQUARES];
extern const enum square square_a2c[N_SQUARES];
extern const enum square square_a2d[N_SQUARES];
extern const castle_rights_t castling_rights[N_PLAYERS][N_BOARDSIDE];

/* Lookup table of permitted slide moves for a rank, given file position and
 * rank occupancy, used to calculate Rook, Bishop and Queen moves */
rank_t permitted_slide_moves[8][256];

/* For B,C,D-stacks, bitboards which convert rank occupancy into a vertical or
   diagonal bitboard in A-plane */
bitboard_t occ_b2a[256], occ_c2a[256], occ_d2a[256];

/*
 * These are reflections in the vertical axis of their C-square counterparts
 * which are generated at runtime.
 */
rank_t shift_d[N_SQUARES];
rank_t mask_d[N_SQUARES];

/* Bitboards for knight and king moves which are generated at runtime. */
bitboard_t knight_moves[N_SQUARES], king_moves[N_SQUARES];

/* Bitboards for pawn advances and takes which are generated at runtime. */
bitboard_t pawn_advances[N_PLAYERS][N_SQUARES],
    pawn_takes[N_PLAYERS][N_SQUARES];

/* Return a bitboard containing the valid rook move destinations for a given
 * position, taking into account captures and blockages by other pieces.  For
 * simplicity, include moves where rooks can take their own side's pieces (these
 * are filtered out elsewhere). */
static bitboard_t get_rook_moves(struct position *position,
                                 enum square a_square) {
  /*
   * Rook move generation reduces to a case for a single 8-bit rank which is
   * handled by an 8 x 256 lookup table.  To calculate horizontal moves, for the
   * rank containing the piece, calculate:
   *  - the shift required to the bitboard to shift the rank down to rank zero
   *  - the file of the moving piece within the rank
   *  - the piece occupancy of other pieces in the rank
   * Then look up the permitted moves for the piece, given the occupancy and
   * file, then shift the permitted moves back onto a bitboard at the original
   * rank. The process is repeated on the 90-degree-rotated B-stack to calculate
   * the vertical moves, and a lookup table is used to convert the moves to a
   * bitboard in the A-stack which is shifted to the correct file, and finally
   * combined with the horizontal moves.
   */
  int a_shift = a_square & ~7ul;
  int a_file = a_square & 7ul;
  rank_t a_occupancy = (rank_t)((position->total_a >> a_shift) & 0xfful);
  rank_t a_moves = permitted_slide_moves[a_file][a_occupancy];
  bitboard_t h_mask = (bitboard_t)a_moves << a_shift;

  int b_square = square_a2b[a_square];
  int b_shift = b_square & ~7ul;
  int b_file = b_square & 7ul;
  rank_t b_occ = (rank_t)((position->total_b >> b_shift) & 0xfful);
  rank_t b_moves = permitted_slide_moves[b_file][b_occ];
  bitboard_t b_mask = occ_b2a[b_moves];
  bitboard_t v_mask = b_mask << a_file;

  return (h_mask | v_mask);
}

/* Return a bitboard containing the valid bishop move destinations for a given
 * position, taking into account captures and blockages by other pieces. For
 * simplicity, include moves where bishops can capture their own side's pieces
 * (these are filtered out elsewhere). */
bitboard_t get_bishop_moves(struct position *position, enum square a_square) {
  /*
   * Move generation for bishops is similar to rooks, but with sliding move
   * calculations performed on the 45-degree-rotated C- and D-stacks.
   * Calculation of shifts and masks is more complicated because the board
   * is diamond-shaped from these perspectives, so the ranks are of different
   * lengths.  Lookup tables are used to get the mask and shift used to isolate
   * the rank, and also to get the bitboard of the moves and the amount it needs
   * to be shifted.
   */
  int c_shift = shift_c[a_square];
  rank_t c_mask = mask_c[a_square];
  int c_file = square_a2c[a_square] - c_shift;
  rank_t c_occ = (rank_t)((position->total_c) >> c_shift) & c_mask;
  rank_t c_moves = permitted_slide_moves[c_file][c_occ] & c_mask;
  bitboard_t c_ret = occ_c2a[c_moves] << shift_c2a[a_square];

  int d_shift = shift_d[a_square];
  rank_t d_mask = mask_d[a_square];
  int d_file = square_a2d[a_square] - d_shift;
  rank_t d_occ = (rank_t)((position->total_d) >> d_shift) & d_mask;
  rank_t d_moves = permitted_slide_moves[d_file][d_occ] & d_mask;
  bitboard_t d_ret = occ_d2a[d_moves] << shift_d2a[a_square];

  return (c_ret | d_ret);
}

/* Return a bitboard containing the valid king move destinations for a given
   position, including any castling destinations. */
bitboard_t get_king_moves(struct position *position, enum square from,
                          enum player player) {
  /*
   * Take all non-castling king moves from a lookup table, removing any that
   * would lead into check.  Return these moves if there are no castling rights
   * or the king is under attack.  Otherwise, for each board side with castling
   * rights, look for any occupied squares which would block the rook from
   * sliding, then any squares under attack which would block the king from
   * sliding.  If there are none, add the castling destinations to the moves.
   */
  bitboard_t moves = king_moves[from] & ~position->claim[opponent[player]];

  if (!(position->castling_rights & castling_rights[player][BOTHSIDES]) ||
      get_attacks(position, from, opponent[player])) {
    return moves;
  }

  bitboard_t all = position->total_a;
  for (int side = 0; side < 2; side++) {
    if (!(position->castling_rights & castling_rights[player][side])) {
      continue;
    }

    bitboard_t blocked = all & rook_castle_slides[player][side];
    if (!blocked) {
      bitboard_t king_slides = king_castle_slides[player][side];
      while (king_slides) {
        bitboard_t mask = take_next_bit_from(&king_slides);
        if (get_attacks(position, bit2square(mask), opponent[player])) {
          blocked |= mask;
        }
      }
    }

    if (!blocked) {
      moves |= castle_destinations[player][side];
    }
  }
  return moves;
}

/* Return a bitboard containing the set of all squares with pieces that
 * can attack a given target square, for a given attacking player. */
bitboard_t get_attacks(const struct position *position, enum square target,
                       enum player attacking) {
  /*
   * This calculation works on the principle that attacking moves are
   * reciprocal, e.g. if a hypothetical knight at the target square could
   * legally attack a certain square, and there is a knight at that square,
   * then that square is also an attacker of the target.  This is applied
   * here to get attackers for all piece types.
   */
  bitboard_t attacks = 0;
  bitboard_t target_mask = square2bit[target];
  int base = attacking * N_PIECE_T;

  /* Check player is not trying to attack own piece (checking an empty square is
   * ok because it needs to be done for castling) */
  ASSERT(position->piece_at[target] == EMPTY ||
         piece_player[(int)position->piece_at[target]] != attacking);

  attacks = pawn_takes[opponent[attacking]][target] & position->a[base + PAWN];
  attacks |= knight_moves[target] & position->a[base + KNIGHT];
  attacks |= king_moves[target] & position->a[base + KING];
  bitboard_t sliders = position->a[base + ROOK] | position->a[base + BISHOP] |
                       position->a[base + QUEEN];
  while (sliders) {
    bitboard_t attacker = take_next_bit_from(&sliders);
    if (target_mask &
        position->moves[(int)position->index_at[bit2square(attacker)]])
      attacks |= attacker;
  }
  return attacks;
}

/* Pre-calculate bitboards within the given position struct containing the set
   of all squares that each piece can move to.  This is called after a move is
   made. */
void calculate_moves(struct position *position) {
  position->claim[0] = 0;
  position->claim[1] = 0;

  /* For each piece apart from kings, get the moves for the piece including
   * taking moves for all pieces */
  for (int8_t index = 0; index < N_PIECES; index++) {
    enum square square = position->piece_square[index];
    if (square == NO_SQUARE) {
      continue;
    }
    ASSERT(position->index_at[square] == index);
    int piece = position->piece_at[square];
    if (piece_type[piece] == KING) continue;

    enum player player = piece_player[piece];
    bitboard_t moves;
    switch (piece_type[piece]) {
      case PAWN: {
        /* Pawns are blocked from moving ahead by any piece */
        bitboard_t block = position->total_a;
        /* Special case for double advance to prevent jumping */
        /* Remove the pawn so it does not block itself */
        block &= ~square2bit[square];
        /* Blocking piece does not just block its own square but also the next
         */
        if (player == WHITE) {
          block |= block << 8;
        } else {
          block |= block >> 8;
        }
        /* Apply block to pawn advances */
        moves = pawn_advances[player][square] & ~block;
        /* Add actual taking moves */
        moves |= pawn_takes[player][square] &
                 (position->player_a[opponent[player]] |
                  (position->en_passant &
                   ((player == BLACK) ? 0xffffffffull : 0xffffffffull << 32)));
        /* Claim for pawns is only taking moves */
        position->claim[player] |=
            pawn_takes[player][square] & ~position->player_a[player];
      } break;
      case ROOK:
        moves = get_rook_moves(position, square);
        break;
      case KNIGHT:
        moves = knight_moves[square];
        break;
      case BISHOP:
        moves = get_bishop_moves(position, square);
        break;
      case QUEEN:
        moves = get_bishop_moves(position, square) |
                get_rook_moves(position, square);
        break;
      case KING:
        /* Player info is required for castling checking */
        moves = get_king_moves(position, square, player);
        break;
      default:
        moves = 0;
        break;
    }
    /* You can't take your own piece */
    moves &= ~position->player_a[player];

    position->moves[index] = moves;
    if (piece_type[piece] != PAWN) {
      position->claim[player] |= moves;
    }
  }

  for (int8_t index = 0; index < N_PIECES; index++) {
    enum square square = position->piece_square[index];
    if (square == NO_SQUARE) {
      continue;
    }
    ASSERT(position->index_at[square] == index);
    int piece = position->piece_at[square];
    if (piece_type[piece] != KING) continue;
    enum player player = piece_player[piece];
    bitboard_t moves =
        get_king_moves(position, position->piece_square[index], player);
    /* You can't take your own piece */
    moves &= ~position->player_a[player];
    position->moves[index] = moves;
    position->claim[player] |= moves;
  }
}

/* Initialises the module */
void init_moves(void) {
  /* Position-indexed tables */
  for (enum square square = 0; square < N_SQUARES; square++) {
    int mirror = (square / 8) * 8 + (7 - square % 8);
    shift_d[square] = shift_c[mirror];
    mask_d[square] = mask_c[mirror];
  }

  /* Table of file 0 occupancies */
  for (int i = 0; i < 256; i++) {
    occ_b2a[i] = 0;
    occ_c2a[i] = 0;
    occ_d2a[i] = 0;
    for (int bit = 0; bit < 8; bit++) {
      if (i & (1 << bit)) {
        occ_b2a[i] |= square2bit[square_a2b[bit]];
        occ_c2a[i] |= square2bit[bit * 7];
        occ_d2a[i] |= square2bit[bit * 9];
      }
    }
  }

  /*
   *  Sliding Moves
   */

  for (enum square square = 0; square < 8; square++) {
    unsigned int rank;
    for (rank = 0; rank < 256; rank++) {
      unsigned int l_mask, r_mask;
      unsigned int l_squares, r_squares, l_pieces, r_pieces;
      int l_first_occupied, r_first_occupied;

      /* 1. Set of all squares to the left of square */
      l_squares = 0xff << (square + 1);
      /* 2. Set of all pieces to the left of square */
      l_pieces = l_squares & rank;
      if (l_pieces) {
        /* 3. First occupied square to the left of square (count trailing
         * zeroes) */
        l_first_occupied = ctz(l_pieces);
        /* 4. Set of all squares to the left of the first occupied */
        l_mask = 0xff << (l_first_occupied + 1);
      } else {
        /* Hack because CTZ is undefined when there are no occupied squares */
        l_mask = 0;
      }

      /* Ditto right hand side, shift the other way and count leading zeroes  */
      r_squares = 0xff >> (8 - square);
      r_pieces = r_squares & rank;
      if (r_pieces) {
        /* Adjust the shift for the size of the word */
        r_first_occupied = sizeof(unsigned int) * 8 - 1 - clz(r_pieces);
        r_mask = 0xff >> (8 - r_first_occupied);
      } else {
        r_mask = 0;
      }

      /* Combine left and right masks and negate to get set of all possible
             moves on the rank. */
      permitted_slide_moves[square][rank] = ~(r_mask | l_mask);
    }
  }

  /*
   *  Knight and King moves
   */

  for (enum square square = 0; square < N_SQUARES; square++) {
    bitboard_t square_mask = square2bit[square];

    /* Possible knight moves: A-H
     * Possible king moves: I-P
     *      A   B
     *    H I J K C
     *      P * L
     *    G O N M D
     *      F   E
     */

    /* BE, KLM */
    if (square_mask & 0x7f7f7f7f7f7f7f7full) {
      knight_moves[square] |= square_mask >> 15;
      knight_moves[square] |= square_mask << 17;
      king_moves[square] |= square_mask << 1;
      king_moves[square] |= square_mask >> 7;
      king_moves[square] |= square_mask << 9;
    }
    /* CD */
    if (square_mask & 0x3f3f3f3f3f3f3f3full) {
      knight_moves[square] |= square_mask >> 6;
      knight_moves[square] |= square_mask << 10;
    }
    /* AF, IOP */
    if (square_mask & 0xfefefefefefefefeull) {
      knight_moves[square] |= square_mask >> 17;
      knight_moves[square] |= square_mask << 15;
      king_moves[square] |= square_mask >> 1;
      king_moves[square] |= square_mask << 7;
      king_moves[square] |= square_mask >> 9;
    }
    /* GH */
    if (square_mask & 0xfcfcfcfcfcfcfcfcull) {
      knight_moves[square] |= square_mask >> 10;
      knight_moves[square] |= square_mask << 6;
    }
    /* JN */
    king_moves[square] |= square_mask >> 8;
    king_moves[square] |= square_mask << 8;
  }

  /*
   *  Pawn moves
   */

  for (enum player player = 0; player < N_PLAYERS; player++) {
    for (enum square square = 0; square < N_SQUARES; square++) {
      bitboard_t current, advance, jump, take;
      /* Mask for pawn */
      current = square2bit[square];
      advance = current;
      /* White and black advance in different directions */
      if (player == WHITE) {
        /* Mask for pawn if it has not been moved */
        jump = advance & starting_a[PAWN];
        /* All pieces can advance by 1 square */
        advance <<= 8;
        /* Advance the pieces not previously moved by 2 squares */
        jump <<= 16;
      } else {
        /* Other direction for black */
        jump = advance & starting_a[PAWN + N_PIECE_T];
        advance >>= 8;
        jump >>= 16;
      }
      /* Set of all possible advance or jump moves */
      pawn_advances[player][square] = advance | jump;

      /* Taking moves */
      take = 0;
      /* If pawn is not on file 0, it can move to a lower file to take */
      if (current & 0x7f7f7f7f7f7f7f7full) {
        take |= advance << 1;
      }
      /* If pawn is not on file 7, it can move to a higher file to take */
      if (current & 0xfefefefefefefefeull) {
        take |= advance >> 1;
      }
      /* Set of all possible taking moves */
      pawn_takes[player][square] = take;
    }
  }
}
