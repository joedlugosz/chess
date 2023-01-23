/*
 *  Functions describing piece moves and relationships
 */

#include <stdlib.h>

#include "debug.h"
#include "io.h"
#include "position.h"

typedef unsigned char rank_t;

const int rook_magic_shift[N_SQUARES] = {
    52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52,
};

const bitboard_t rook_magic_number[N_SQUARES] = {
    0x1080082040008010, 0x0040400020001000, 0x0080098020003004,
    0x0100081001600500, 0x20800a0800040080, 0x0080010400020080,
    0x0280010002002080, 0x0080004100002480, 0xa000800080204010,
    0x8000400250002009, 0x0008802002801000, 0x0001801000180080,
    0x1000808008000400, 0x8012800400820080, 0x0882000401020008,
    0x4100800100284080, 0x0080014000a00040, 0x08c0004020100040,
    0x0001010040200010, 0xa401010010002008, 0x0080808004010800,
    0x0200808004000200, 0x0000840010150802, 0x0000020006840841,
    0x0000400880208001, 0xa400200040005000, 0x0000120600208040,
    0x0010040140080040, 0x0088880080140080, 0x0001000700084400,
    0x4100100400080102, 0x0002408200004401, 0x8001804000800020,
    0x409000a000400840, 0x0810100080802000, 0x1005002009001000,
    0x4a00801800800400, 0x0100804201800400, 0x0000800200800100,
    0x09000100c2000084, 0x1000904000208002, 0x0000400020008080,
    0x0030a00010008080, 0x00021042000a0020, 0x000800800c008008,
    0x0002001008020084, 0x0000210002008080, 0x0001000084410002,
    0x9100800040003080, 0x0000200184c00080, 0x0009009020004300,
    0x0000080280100080, 0x0101080045001100, 0x0084044020100801,
    0x0200020801101c00, 0x0080110041840200, 0x0010102040800301,
    0x2020204000810011, 0x0000200018104501, 0x0000042009001001,
    0x0002001004082102, 0x8002002804100102, 0x0000028910020804,
    0x0000010084007042,
};

const bitboard_t rook_magic_mask[N_SQUARES] = {
    0x000101010101017e, 0x000202020202027c, 0x000404040404047a,
    0x0008080808080876, 0x001010101010106e, 0x002020202020205e,
    0x004040404040403e, 0x008080808080807e, 0x0001010101017e00,
    0x0002020202027c00, 0x0004040404047a00, 0x0008080808087600,
    0x0010101010106e00, 0x0020202020205e00, 0x0040404040403e00,
    0x0080808080807e00, 0x00010101017e0100, 0x00020202027c0200,
    0x00040404047a0400, 0x0008080808760800, 0x00101010106e1000,
    0x00202020205e2000, 0x00404040403e4000, 0x00808080807e8000,
    0x000101017e010100, 0x000202027c020200, 0x000404047a040400,
    0x0008080876080800, 0x001010106e101000, 0x002020205e202000,
    0x004040403e404000, 0x008080807e808000, 0x0001017e01010100,
    0x0002027c02020200, 0x0004047a04040400, 0x0008087608080800,
    0x0010106e10101000, 0x0020205e20202000, 0x0040403e40404000,
    0x0080807e80808000, 0x00017e0101010100, 0x00027c0202020200,
    0x00047a0404040400, 0x0008760808080800, 0x00106e1010101000,
    0x00205e2020202000, 0x00403e4040404000, 0x00807e8080808000,
    0x007e010101010100, 0x007c020202020200, 0x007a040404040400,
    0x0076080808080800, 0x006e101010101000, 0x005e202020202000,
    0x003e404040404000, 0x007e808080808000, 0x7e01010101010100,
    0x7c02020202020200, 0x7a04040404040400, 0x7608080808080800,
    0x6e10101010101000, 0x5e20202020202000, 0x3e40404040404000,
    0x7e80808080808000,
};

const int bishop_magic_shift[N_SQUARES] = {
    58, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 58,
};

const bitboard_t bishop_magic_number[N_SQUARES] = {
    0x0002080104018200, 0x0088020800410010, 0x0450010041000200,
    0x2004040284000000, 0x0044142000000008, 0x0000901008002000,
    0x0801141002080000, 0x0001040064040404, 0x0000102008014040,
    0x00000808080040c0, 0x0020041104010008, 0x0410020a02040001,
    0x0000020a10200100, 0x2000009004204000, 0x0101010090042020,
    0x001002020a010c00, 0x0120000620020220, 0x0003001024030400,
    0x00404202020a0020, 0x0008000082014040, 0x1001000090400082,
    0x100040a080602000, 0x8000a00402080210, 0x2002200202010480,
    0x0002920028200800, 0x0112080010100880, 0x0008050002220200,
    0x0044010008200880, 0x040094000080a000, 0x0008004822020a00,
    0x0002005402011000, 0x0001020000424401, 0x0102114004040800,
    0x0808080c08080100, 0x0002018204101020, 0x5200020080080080,
    0x00a8408020020200, 0x1001080200006200, 0x0001220402408400,
    0x0088010020010280, 0x0002180208084000, 0x0005081804000200,
    0x0023008041003000, 0x0400104208000080, 0x0001401009084080,
    0x0001020281000200, 0x0060040900400208, 0x0004018405000140,
    0x0001040220050400, 0x0001004802080000, 0x0800044044100001,
    0x1004020084040000, 0x1020020821010200, 0x0800202801084400,
    0x02c0048104010000, 0x0020084108408800, 0x0006820042200400,
    0x0010042a08040400, 0x0008000040441000, 0x0040000140840408,
    0x0001004240090240, 0x0000010820080180, 0x0008100a10010200,
    0x0202200200810100,
};

const bitboard_t bishop_magic_mask[N_SQUARES] = {
    0x0040201008040200, 0x0000402010080400, 0x0000004020100a00,
    0x0000000040221400, 0x0000000002442800, 0x0000000204085000,
    0x0000020408102000, 0x0002040810204000, 0x0020100804020000,
    0x0040201008040000, 0x00004020100a0000, 0x0000004022140000,
    0x0000000244280000, 0x0000020408500000, 0x0002040810200000,
    0x0004081020400000, 0x0010080402000200, 0x0020100804000400,
    0x004020100a000a00, 0x0000402214001400, 0x0000024428002800,
    0x0002040850005000, 0x0004081020002000, 0x0008102040004000,
    0x0008040200020400, 0x0010080400040800, 0x0020100a000a1000,
    0x0040221400142200, 0x0002442800284400, 0x0004085000500800,
    0x0008102000201000, 0x0010204000402000, 0x0004020002040800,
    0x0008040004081000, 0x00100a000a102000, 0x0022140014224000,
    0x0044280028440200, 0x0008500050080400, 0x0010200020100800,
    0x0020400040201000, 0x0002000204081000, 0x0004000408102000,
    0x000a000a10204000, 0x0014001422400000, 0x0028002844020000,
    0x0050005008040200, 0x0020002010080400, 0x0040004020100800,
    0x0000020408102000, 0x0000040810204000, 0x00000a1020400000,
    0x0000142240000000, 0x0000284402000000, 0x0000500804020000,
    0x0000201008040200, 0x0000402010080400, 0x0002040810204000,
    0x0004081020400000, 0x000a102040000000, 0x0014224000000000,
    0x0028440200000000, 0x0050080402000000, 0x0020100804020000,
    0x0040201008040200,
};

typedef bitboard_t (*generate_moves_fn)(enum square, bitboard_t);
bitboard_t *rook_magic_moves;
bitboard_t *bishop_magic_moves;

/* clang-format off */

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
static inline bitboard_t get_rook_moves(bitboard_t occupancy,
                                        enum square square) {
  return rook_magic_moves[square * 4096 +
                          (((occupancy & rook_magic_mask[square]) *
                            rook_magic_number[square]) >>
                           rook_magic_shift[square])];
}

/* Return a bitboard containing the valid bishop move destinations for a given
 * position, taking into account captures and blockages by other pieces. For
 * simplicity, include moves where bishops can capture their own side's pieces
 * (these are filtered out elsewhere). */
static inline bitboard_t get_bishop_moves(bitboard_t occupancy,
                                          enum square square) {
  return bishop_magic_moves[square * 4096 +
                            (((occupancy & bishop_magic_mask[square]) *
                              bishop_magic_number[square]) >>
                             bishop_magic_shift[square])];
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
  position->claim[WHITE] = 0;
  position->claim[BLACK] = 0;

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
      case PAWN:
        moves = get_pawn_moves(position, square, player);
        break;
      case ROOK:
        moves = get_rook_moves(position->total_a, square);
        break;
      case KNIGHT:
        moves = knight_moves[square];
        break;
      case BISHOP:
        moves = get_bishop_moves(position->total_a, square);
        break;
      case QUEEN:
        moves = get_bishop_moves(position->total_a, square) |
                get_rook_moves(position->total_a, square);
        break;
      case KING:
        moves = get_king_moves(position, square, player);
        break;
      default:
        moves = 0;
        break;
    }

    position->moves[index] = moves;

    if (piece_type[piece] != PAWN) {
      position->claim[player] |= moves & ~position->player_a[player];
    } else {
      position->claim[player] |=
          pawn_takes[player][square] & ~position->player_a[player];
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
    position->moves[index] = moves;
    position->claim[player] |= moves & ~position->player_a[player];
  }
}

bitboard_t generate_rook_moves(enum square square, bitboard_t blockers) {
  int rank = square / 8;
  int file = square % 8;
  bitboard_t moves = 0ull;
  for (int i = rank + 1; i < 8; i++) {
    bitboard_t bit = 1ull << (i * 8 + file);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int i = rank - 1; i >= 0; i--) {
    bitboard_t bit = 1ull << (i * 8 + file);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int i = file + 1; i < 8; i++) {
    bitboard_t bit = 1ull << (rank * 8 + i);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int i = file - 1; i >= 0; i--) {
    bitboard_t bit = 1ull << (rank * 8 + i);
    moves |= bit;
    if (blockers & bit) break;
  }
  return moves;
}

bitboard_t generate_bishop_moves(enum square square, bitboard_t blockers) {
  int rank = square / 8;
  int file = square % 8;
  bitboard_t moves = 0ull;
  for (int r = rank + 1, f = file + 1; r < 8 && f < 8; r++, f++) {
    bitboard_t bit = 1ull << (r * 8 + f);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int r = rank - 1, f = file - 1; r >= 0 && f >= 0; r--, f--) {
    bitboard_t bit = 1ull << (r * 8 + f);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int r = rank - 1, f = file + 1; r >= 0 && f < 8; r--, f++) {
    bitboard_t bit = 1ull << (r * 8 + f);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int r = rank + 1, f = file - 1; r < 8 && f >= 0; r++, f--) {
    bitboard_t bit = 1ull << (r * 8 + f);
    moves |= bit;
    if (blockers & bit) break;
  }
  return moves;
}

bitboard_t unpack_moves(bitboard_t template, bitboard_t packed) {
  bitboard_t unpacked = 0;
  while (template) {
    bitboard_t bit = take_next_bit_from(&template);
    if (packed & 1) {
      unpacked |= bit;
    }
    packed >>= 1;
  }
  return unpacked;
}

static void init_magic_moves(generate_moves_fn generate_moves, const int *shift,
                             const bitboard_t *magic_mask,
                             const bitboard_t *magic_number,
                             bitboard_t *magic_moves) {
  for (enum square square = A1; square <= H8; square++) {
    bitboard_t blocker_mask = magic_mask[square];
    int n_bits = pop_count(blocker_mask);
    for (bitboard_t occupancy = 0; occupancy < (1 << n_bits); occupancy++) {
      bitboard_t blocker_board = unpack_moves(blocker_mask, occupancy);
      bitboard_t hash = (blocker_board * magic_number[square]) >> shift[square];
      bitboard_t *entry = &magic_moves[square * 4096 + hash];
      if (!*entry) *entry = generate_moves(square, blocker_board);
    }
  }
}

/* Initialise the module and pre-calculated lookup tables. */
void init_moves(void) {
  rook_magic_moves = malloc(N_SQUARES * 4096 * sizeof(*rook_magic_moves));
  init_magic_moves(generate_rook_moves, rook_magic_shift, rook_magic_mask,
                   rook_magic_number, rook_magic_moves);
  bishop_magic_moves = malloc(N_SQUARES * 4096 * sizeof(*bishop_magic_moves));
  init_magic_moves(generate_bishop_moves, bishop_magic_shift, bishop_magic_mask,
                   bishop_magic_number, bishop_magic_moves);

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

static int check_piece_magics(generate_moves_fn generate_moves,
                              const int *shift, const bitboard_t *blocker_mask,
                              const bitboard_t *magics,
                              const bitboard_t *magic_moves) {
  for (int square = 0; square < 64; square++) {
    for (bitboard_t test = 1; test < 4096; test++) {
      bitboard_t occ = unpack_moves(blocker_mask[square], test);
      bitboard_t g_moves = generate_moves(square, occ);
      bitboard_t hash = (occ * magics[square]) >> shift[square];
      bitboard_t m_moves = magic_moves[square * 4096 + hash];
      if (m_moves != g_moves) {
        printf("Occupancy:\n");
        print_board(0, occ, 1ull << square);
        printf("Generated moves:\n");
        print_board(0, g_moves, 0);
        printf("Magic moves:\n");
        print_board(0, m_moves, 0);
        return 1;
      }
    }
  }
  return 0;
}

int check_magics() {
  return check_piece_magics(generate_rook_moves, rook_magic_shift,
                            rook_magic_mask, rook_magic_number,
                            rook_magic_moves) |
         check_piece_magics(generate_bishop_moves, bishop_magic_shift,
                            bishop_magic_mask, bishop_magic_number,
                            bishop_magic_moves);
}