/*
 *  Functions describing piece moves and relationships
 */

#include "debug.h"
#include "io.h"
#include "position.h"

typedef unsigned char rank_t;

/* clang-format off */

/* 
 * Lookup tables - for each A-square, the shift amount that the C-stack must be
 * shifted to get the start of a diagonal row, and the mask that must be applied
 * following the shift to obtain only the row.
 *
 *     A square  Shift     Mask 
 *               ...etc
 *      9 8 7 6  6         0x0f
 *       5 4 3   3         0x07
 *        2 1    1         0x03
 *         0     0         0x01
 *
 * Reflections in the vertical axis for D-square counterparts are generated by
 * `init_moves`.
 */
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
rank_t shift_d[N_SQUARES];
rank_t mask_d[N_SQUARES];

/* 
 * Lookup tables - Amount that occupancy mask occ_c2a[occ] must be shifted to
 * get to the correct diagonal
 */
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
  /* Pawns     Rooks        Bishops      Knights      Queen        King                 */
  0xffull<<8,  0x81ull,     0x42ull,     0x24ull,     0x08ull,     0x10ull,    /* White */
  0xffull<<48, 0x81ull<<56, 0x42ull<<56, 0x24ull<<56, 0x08ull<<56, 0x10ull<<56 /* Black */
};

/* clang-format on */

/* For each player and board side, the set of squares a king needs to slide
 * through to castle. */
const bitboard_t king_castle_slides[N_PLAYERS][2] = {
    {0x0cull, 0x60ull}, {0x0cull << 56, 0x60ull << 56}};

/* For each player and board side, the set of squares a rook needs to slide
 * through to castle. */
const bitboard_t rook_castle_slides[N_PLAYERS][2] = {
    {0x0eull, 0x60ull}, {0x0eull << 56, 0x60ull << 56}};

/* For each player and board side, the destination of the king when making a
 * castling move. */
const bitboard_t castle_destinations[N_PLAYERS][2] = {
    {0x04ull, 0x40ull}, {0x04ull << 56, 0x40ull << 56}};

extern const enum square square_a2b[N_SQUARES];
extern const enum square square_a2c[N_SQUARES];
extern const enum square square_a2d[N_SQUARES];
extern const castle_rights_t castling_rights[N_PLAYERS][N_BOARDSIDE];

/* Lookup table of permitted slide moves for a rank, given file position and
 * rank occupancy, generated  by `init_moves`, used to calculate Rook, Bishop
 * and Queen moves */
rank_t permitted_slide_moves[8][256];

/* For B,C,D-stacks, bitboards which convert rank occupancy into a vertical or
   diagonal bitboard in A-plane, which are generated by `init_moves` */
bitboard_t occ_b2a[256], occ_c2a[256], occ_d2a[256];

/* Bitboards for knight and king moves which are generated by `init_moves`. */
bitboard_t knight_moves[N_SQUARES], king_moves[N_SQUARES];

/* Bitboards for pawn advances and takes which are generated by `init_moves`. */
bitboard_t pawn_advances[N_PLAYERS][N_SQUARES],
    pawn_takes[N_PLAYERS][N_SQUARES];

/* Return a bitboard containing the valid pawn move destinations for a given
 * position, taking into account captures and blockages by other pieces.  For
 * simplicity, include moves where pawns can take their own side's pieces (these
 * are filtered out elsewhere). */
static bitboard_t get_pawn_moves(struct position *position, enum square square,
                                 enum player player) {
  /*
   * Pawns can advance forward by one or two squares as dictated by
   * `pawn_advances`, if they not are blocked from moving by other pieces.  A
   * set of blocking pieces is constructed from the total pieces, and it is
   * shifted forward by a rank and combined with itself so that it blocks double
   * advances.  The position of the pawn is removed first so that it doesn't
   * create a mask blocking itself.  The blocking mask is applied to
   * `pawn_advances` to get the advancing moves.  Capturing moves are added
   * where the lookup from `pawn_takes` coincides with opponents pieces, or
   * where there is an en-passant square on the opponent's side of the board.
   */
  bitboard_t block = position->total_a & ~square2bit[square];
  if (player == WHITE) {
    block |= block << 8;
  } else {
    block |= block >> 8;
  }
  bitboard_t moves = pawn_advances[player][square] & ~block;
  moves |= pawn_takes[player][square] &
           (position->player_a[opponent[player]] |
            (position->en_passant &
             ((player == BLACK) ? 0xffffffffull : 0xffffffffull << 32)));
  return moves;
}

/* Return a bitboard containing the valid rook move destinations for a given
 * position, taking into account captures and blockages by other pieces.  For
 * simplicity, include moves where rooks can take their own side's pieces (these
 * are filtered out elsewhere). */
static bitboard_t get_rook_moves(const struct position *position,
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
  rank_t b_occupancy = (rank_t)((position->total_b >> b_shift) & 0xfful);
  rank_t b_moves = permitted_slide_moves[b_file][b_occupancy];
  bitboard_t b_mask = occ_b2a[b_moves];
  bitboard_t v_mask = b_mask << a_file;

  return (h_mask | v_mask);
}

/* Return a bitboard containing the valid bishop move destinations for a given
 * position, taking into account captures and blockages by other pieces. For
 * simplicity, include moves where bishops can capture their own side's pieces
 * (these are filtered out elsewhere). */
bitboard_t get_bishop_moves(const struct position *position,
                            enum square a_square) {
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
   * Non-castling king moves are taken from a lookup table, removing any that
   * would lead into check.  These are returned if there are no castling rights
   * or the king is under attack.  Otherwise, for each board side with castling
   * rights, there is a search for any occupied squares which would block the
   * rook from sliding, then any squares under attack which would block the king
   * from sliding.  If there are none, castling destinations are added to the
   * set of moves.
   */
  bitboard_t moves = king_moves[from] & ~position->claim[opponent[player]];

  if (!(position->castling_rights & castling_rights[player][BOTHSIDES]) ||
      square2bit[from] & position->claim[!player]) {
    return moves;
  }

  bitboard_t all = position->total_a;
  for (int side = 0; side < 2; side++) {
    if (!(position->castling_rights & castling_rights[player][side])) {
      continue;
    }

    bitboard_t blocked = all & rook_castle_slides[player][side];
    bitboard_t attacked =
        position->claim[!player] & king_castle_slides[player][side];

    if (!(blocked | attacked)) moves |= castle_destinations[player][side];
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
   * here to get attackers for all piece types.  When the sliding moves are
   * pre-calculated, it is ~10% quicker to iterate over all the sliders and use
   * their moves to find the king, than to compute bishop and rook moves for the
   * king.
   */
  /* Check that player is not trying to attack own piece (checking for attacks
   * on an empty square is ok because it needs to be done for castling) */
  ASSERT(position->piece_at[target] == EMPTY ||
         piece_player[(int)position->piece_at[target]] != attacking);

  bitboard_t attacks = 0;
  int base = attacking * N_PIECE_T;

  attacks = pawn_takes[opponent[attacking]][target] & position->a[base + PAWN];
  attacks |= knight_moves[target] & position->a[base + KNIGHT];
  attacks |= king_moves[target] & position->a[base + KING];
  bitboard_t sliders = position->a[base + ROOK] | position->a[base + BISHOP] |
                       position->a[base + QUEEN];
  while (sliders) {
    bitboard_t attacker = take_next_bit_from(&sliders);
    if (square2bit[target] &
        position->moves[(int)position->index_at[bit2square(attacker)]])
      attacks |= attacker;
  }
  return attacks;
}

/* Pre-calculate bitboards within the given position struct containing the set
   of all squares that each piece can move to.  Also pre-calculate a claim for
   each player.  This is called after a move is made. */
void calculate_moves(struct position *position) {
  /*
   * For each piece apart from kings, moves are calculated for the piece
   * including capturing moves.  Moves where the player captures
   * their own piece are filtered out here.  Claim is updated for each side,
   * which is the set of all squares that the side could potentially move to by
   * capture.  For pawns, the added claim is only the capturing moves, for other
   * pieces, all moves are added.  King moves are handled last, because
   * generation depends on `get_attacks` which requires move information for all
   * other pieces.
   */

  int base = 0;
  for (enum player player = WHITE; player != N_PLAYERS; player++) {
    position->claim[player] = 0;
    bitboard_t pawns = position->a[base + PAWN];
    while (pawns) {
      bitboard_t piece = take_next_bit_from(&pawns);
      enum square square = bit2square(piece);
      int index = position->index_at[square];
      bitboard_t moves = get_pawn_moves(position, square, player);
      moves &= ~position->player_a[player];
      position->moves[index] = moves;
      position->claim[player] |=
          pawn_takes[player][square] & ~position->player_a[player];
    }
    bitboard_t knights = position->a[base + KNIGHT];
    if (knights) {
      while (knights) {
        bitboard_t piece = take_next_bit_from(&knights);
        enum square square = bit2square(piece);
        int index = position->index_at[square];
        bitboard_t moves = knight_moves[square];
        moves &= ~position->player_a[player];
        position->moves[index] = moves;
        position->claim[player] |= moves;
      }
    }
    bitboard_t rooks = position->a[base + ROOK];
    while (rooks) {
      bitboard_t piece = take_next_bit_from(&rooks);
      enum square square = bit2square(piece);
      int index = position->index_at[square];
      bitboard_t moves = get_rook_moves(position, square);
      moves &= ~position->player_a[player];
      position->moves[index] = moves;
      position->claim[player] |= moves;
    }
    bitboard_t bishops = position->a[base + BISHOP];
    while (bishops) {
      bitboard_t piece = take_next_bit_from(&bishops);
      enum square square = bit2square(piece);
      int index = position->index_at[square];
      bitboard_t moves = get_bishop_moves(position, square);
      moves &= ~position->player_a[player];
      position->moves[index] = moves;
      position->claim[player] |= moves;
    }
    bitboard_t queens = position->a[base + QUEEN];
    if (queens) {
      while (queens) {
        bitboard_t piece = take_next_bit_from(&queens);
        enum square square = bit2square(piece);
        int index = position->index_at[square];
        bitboard_t moves = get_bishop_moves(position, square) |
                           get_rook_moves(position, square);
        moves &= ~position->player_a[player];
        position->moves[index] = moves;
        position->claim[player] |= moves;
      }
    }
    base = N_PIECE_T;
  }

  for (enum player player = WHITE; player != N_PLAYERS; player++) {
    enum square square =
        bit2square(position->a[player ? KING + N_PIECE_T : KING]);
    int index = position->index_at[square];
    bitboard_t moves = get_king_moves(position, square, player);
    moves &= ~position->player_a[player];
    position->moves[index] = moves;
    position->claim[player] |= moves;
  }
}

/* Initialise the module and pre-calculated lookup tables. */
void init_moves(void) {
  /*
   * Position-indexed tables
   */
  for (enum square square = 0; square < N_SQUARES; square++) {
    int mirror = (square / 8) * 8 + (7 - square % 8);
    shift_d[square] = shift_c[mirror];
    mask_d[square] = mask_c[mirror];
  }

  /*
   * Bitboard occupancies
   *
   * Generate bitboard lookup tables which convert rank occupancy into a
   * vertical or diagonal bitboard in A-plane.
   */

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
   * Sliding destinations
   *
   * Generate an 8 x 256 lookup table of permitted sliding destinations within a
   * single rank.  For each square and rank occupancy, get the set of all pieces
   * to the left of the target square, find the first occupied square to the
   * left of the target square, then get the set of all squares to the left of
   * the first occupied square, which are impossible to move to by slide move.
   * Do the same for pieces to the right of the square, and combine and invert
   * them to produce the set of permitted sliding destinations.  The first
   * occupied square is found using CTZ/CLZ operations, with a special case for
   * no occupied squares, because these operations are undefined for zero.  For
   * the CLZ operation, the number of leading zeros must be reduced from the
   * word size down to the rank size.
   */

  for (enum square square = 0; square < 8; square++) {
    unsigned int rank;
    for (rank = 0; rank < 256; rank++) {
      unsigned int l_squares = 0xff << (square + 1);
      unsigned int l_pieces = l_squares & rank;
      unsigned int l_impossible;
      if (l_pieces) {
        int l_first_occupied = ctz(l_pieces);
        l_impossible = 0xff << (l_first_occupied + 1);
      } else {
        l_impossible = 0;
      }

      unsigned int r_squares = 0xff >> (8 - square);
      unsigned int r_pieces = r_squares & rank;
      unsigned int r_impossible;
      if (r_pieces) {
        int r_first_occupied =
            sizeof(unsigned long long) * 8 - 1 - clz(r_pieces);
        r_impossible = 0xff >> (8 - r_first_occupied);
      } else {
        r_impossible = 0;
      }

      permitted_slide_moves[square][rank] = ~(r_impossible | l_impossible);
    }
  }

  /*
   * Knight and King Moves
   *
   * Generate lookup tables for knight and king moves. For each square, the king
   * and knight moves are neighbouring squares as shown below.  Moves are added
   * in turn by shifting a mask of the square, but only if the square is far
   * enough from the edge of the board that the resulting moves don't wrap
   * around between ranks.
   *
   * knight_moves[square]: A-H
   * king_moves[square]:   I-P
   * square2bit[square]:   #
   *
   *      A   B
   *    H I J K C
   *      P # L
   *    G O N M D
   *      F   E
   */

  for (enum square square = 0; square < N_SQUARES; square++) {
    bitboard_t square_mask = square2bit[square];

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
   * Pawn moves
   *
   * Generate bitboard lookup tables for forward pawn advance and diagonal
   * capture moves for each player and square.  White and black need separate
   * tables because they advance in different directions.  All pawns can advance
   * forward by 1 square, and pawns in starting positions can jump advance by 2
   * squares. `pawn_advances` is the set of all possible advance or jump moves.
   * Capturing moves are generated from the single advance moves, shifted left
   * or right, provided that they don't overflow off the edge of the board.
   */

  for (enum player player = 0; player < N_PLAYERS; player++) {
    for (enum square square = 0; square < N_SQUARES; square++) {
      bitboard_t pawn_bit = square2bit[square];

      bitboard_t advance, jump;
      if (player == WHITE) {
        advance = pawn_bit << 8;
        jump = (pawn_bit & starting_a[PAWN]) << 16;
      } else {
        advance = pawn_bit >> 8;
        jump = (pawn_bit & starting_a[PAWN + N_PIECE_T]) >> 16;
      }
      pawn_advances[player][square] = advance | jump;

      bitboard_t take = 0;
      if (pawn_bit & 0x7f7f7f7f7f7f7f7full) {
        take |= advance << 1;
      }
      if (pawn_bit & 0xfefefefefefefefeull) {
        take |= advance >> 1;
      }
      pawn_takes[player][square] = take;
    }
  }
}
