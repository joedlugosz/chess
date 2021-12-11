/*
 *  Piece moves and relationships
 */

#include "state.h"
#include "log.h"
#include "io.h"

typedef unsigned char rank_t;

/* For each A-pos, the amount that the C-stack must be
   shifted to get the start of a diagonal row */
const pos_t shift_c[N_POS] = {
   0,  1,  3,  6, 10, 15, 21, 28,
   1,  3,  6, 10, 15, 21, 28, 36,
   3,  6, 10, 15, 21, 28, 36, 43,
   6, 10, 15, 21, 28, 36, 43, 49,
  10, 15, 21, 28, 36, 43, 49, 54,
  15, 21, 28, 36, 43, 49, 54, 58,
  21, 28, 36, 43, 49, 54, 58, 61,
  28, 36, 43, 49, 54, 58, 61, 63
};
/* For each A-pos, the mask that must be applied to 
   obtain a single diagonal row */
const rank_t mask_c[N_POS] = {
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
const pos_t shift_c2a[N_POS] = {
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
const pos_t shift_d2a[N_POS] = {
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

const bitboard_t king_castle_slides[N_PLAYERS][2] = { { 0x0cull, 0x60ull }, { 0x0cull << 56, 0x60ull << 56 } };
const bitboard_t rook_castle_slides[N_PLAYERS][2] = { { 0x0eull, 0x60ull }, { 0x0eull << 56, 0x60ull << 56 } };
const bitboard_t castle_destinations[N_PLAYERS][2] = { { 0x04ull, 0x40ull }, { 0x04ull << 56, 0x40ull << 56 } };

extern const pos_t pos_a2b[N_POS];
extern const pos_t pos_a2c[N_POS];
extern const pos_t pos_a2d[N_POS];
extern const castle_rights_t castling_rights[N_PLAYERS][N_BOARDSIDE];

/* Rank blocking table for Rook, Bishop and Queen moves */
unsigned int blocking[8][256];
/* For B,C,D-stacks, masks taking rank occupancy back into a vertical or 
   diagonal in A-plane */
bitboard_t occ_b2a[256], occ_c2a[256], occ_d2a[256];

/* These are reflections in the vertical axis of their C-pos counterparts 
   which are calculated at run-time */
rank_t shift_d[N_POS];
rank_t mask_d[N_POS];

/* Knight and king moves */
bitboard_t knight_moves[N_POS], king_moves[N_POS];
/* Pawn advances and takes */
bitboard_t pawn_advances[N_PLAYERS][N_POS], pawn_takes[N_PLAYERS][N_POS];

/* Returns the valid rook moves for a given position, taking into account 
   blockages by other pieces.  Also returns moves where rooks can take their 
   own side's pieces (these are filtered out elsewhere). */
bitboard_t get_rook_moves(state_s *state, pos_t a_pos)
{
  /* A-stack */
  /* The shift that is required to move the rank containing pos down to rank zero */
  int a_shift = a_pos & ~7ul;
  /* Position within that rank */
  int a_file = a_pos & 7ul;
  /* Piece occupancy of the rank in the A-stack */
  rank_t a_occ = (rank_t)((state->total_a >> a_shift) & 0xfful);
  /* Permitted moves within the rank */
  rank_t a_moves = blocking[a_file][a_occ];
  /* A-mask of permitted horizontal moves */
  bitboard_t h_mask = (bitboard_t)a_moves << a_shift;  
  /* B-stack */
  /* B-position */
  int b_pos = pos_a2b[a_pos];
  /* Shift required to shift the B-rank containing pos down to rank zero */
  int b_shift = b_pos & ~7ul;
  int b_file = b_pos & 7ul;
  rank_t b_occ = (rank_t)((state->total_b >> b_shift) & 0xfful);
  rank_t b_moves = blocking[b_file][b_occ];
  /* A-mask (note) of permitted vertical moves in file 0 */
  bitboard_t b_mask = occ_b2a[b_moves];
  /* Shift A-mask to correct file position */
  bitboard_t v_mask = b_mask << a_file;
  /* Or the horizontal and vertical masks */
  return (h_mask | v_mask); 
}

/* Returns the valid bishop moves for a given position, taking into account 
   blockages by other pieces.  Also returns moves where bishops can take their
   own side's pieces (these are filtered out elsewhere). */
bitboard_t get_bishop_moves(state_s *state, pos_t a_pos)
{
  /* C-pos */
  /* Shift required to shift the rank containing pos down to rank zero */
  int c_shift = shift_c[a_pos];
  rank_t c_mask = mask_c[a_pos];
  /* Effective file within C-rank is calculated from the pos and the shift */
  int c_file = pos_a2c[a_pos] - c_shift;
  /* Piece occupancy of the rank in the C-stack */
  rank_t c_occ = (rank_t)((state->total_c) >> c_shift) & c_mask;
  /* Permitted moves within the rank */
  rank_t c_moves = blocking[c_file][c_occ] & c_mask;
  /* A-mask of permitted C moves */
  bitboard_t c_ret = occ_c2a[c_moves] << shift_c2a[a_pos];
  /* D-pos */
  /* Shift required to shift the rank containing pos down to rank zero */
  int d_shift = shift_d[a_pos];
  rank_t d_mask = mask_d[a_pos];
  /* Effective file within C-rank is calculated from the pos and the shift */
  int d_file = pos_a2d[a_pos] - d_shift;
  /* Piece occupancy of the rank in the D-stack */
  rank_t d_occ = (rank_t)((state->total_d) >> d_shift) & d_mask;
  /* Permitted moves within the rank */
  rank_t d_moves = blocking[d_file][d_occ] & d_mask;
  /* A-mask of permitted D moves */
  bitboard_t d_ret = occ_d2a[d_moves] << shift_d2a[a_pos];

  return (c_ret | d_ret); 
}

/* Returns the valid king moves for a given position, including king 
   destinations for castling if possible */
bitboard_t get_king_moves(state_s *state, pos_t from, player_e player)
{
  /* All non-castling king moves minus any that would lead into check */
  bitboard_t moves = king_moves[from] & ~state->claim[opponent[player]];

  /* Castling */
  /* There must be some castling rights for this player and the king must not
   * be under attack. */
  if(!(state->castling_rights & castling_rights[player][BOTHSIDES])) {
    return moves;
  }
  if(get_attacks(state, from, opponent[player])) {
    return moves;
  }
  /* Get all pieces in the board which might block castling */
  bitboard_t all = state->total_a;
  for(int side = 0; side < 2; side++) {
    /* Castling rights must exist for this board side */
    if(!(state->castling_rights & castling_rights[player][side])) {
      continue;
    }
    /* Look for any occupied sqares which would block the rook from
     * sliding, then any squares under attack which would block the king
     * from sliding.  */
    bitboard_t blocked = all & rook_castle_slides[player][side];
    if(!blocked) {
      bitboard_t king_slides = king_castle_slides[player][side];
      while(king_slides) {
        bitboard_t mask = next_bit_from(&king_slides);
        if(get_attacks(state, mask2pos(mask), opponent[player])) {
          blocked |= mask;
        }
      }
    }
    /* If it is possible to castle add the destination as a valid move */
    if(!blocked) {
      moves |= castle_destinations[player][side];
    }
  }
  return moves;
}

/* Returns the set of all pieces that can attack a given position */
bitboard_t get_attacks(state_s *state, pos_t target, player_e attacking)
{
  bitboard_t attacks = 0;
  bitboard_t target_mask = pos2mask[target];
  int base = attacking * N_PIECE_T;
  /* Check player is not trying to attack own piece (checking an empty square is ok because
   * it needs to be done for castling) */
  ASSERT(state->piece_at[target] == EMPTY ||
    piece_player[(int)state->piece_at[target]] != attacking);
  /* Note - viewed as if the non-attacking player is attacking */
  attacks = pawn_takes[opponent[attacking]][target] & state->a[base + PAWN];
  attacks |= knight_moves[target] & state->a[base + KNIGHT];
  attacks |= king_moves[target] & state->a[base + KING];
  bitboard_t sliders = state->a[base + ROOK] | state->a[base + BISHOP] | state->a[base + QUEEN];
  while(sliders) {
    bitboard_t attacker = next_bit_from(&sliders);
    if(target_mask & state->moves[(int)state->index_at[mask2pos(attacker)]]) attacks |= attacker;
  }
  return attacks;
}

/* Returns the set of all positions that a piece can move to, given the
   position of the moving piece. */
void calculate_moves(state_s *state)
{
  state->claim[0] = 0;
  state->claim[1] = 0;

  for(int8_t index = 0; index < N_PIECES; index++) {
    pos_t pos = state->piece_pos[index];      
    if(pos == NO_POS) {
      continue;
    }
    ASSERT(state->index_at[pos] == index);
    int piece = state->piece_at[pos];
    /* King handled at the end */
    if(piece_type[piece] == KING) 
      continue;
    player_e player = piece_player[piece];
    bitboard_t moves;
    /* Get moves for the piece including taking moves for all pieces */
    switch(piece_type[piece]) {	
    case PAWN:
      {
        /* Pawns are blocked from moving ahead by any piece */
        bitboard_t block = state->total_a;
        /* Special case for double advance to prevent jumping */
        /* Remove the pawn so it does not block itself */
        block &= ~pos2mask[pos];
        /* Blocking piece does not just block its own square but also the next */
        if(player == WHITE) {
          block |= block << 8;
        } else {
          block |= block >> 8;
        }
        /* Apply block to pawn advances */
        moves = pawn_advances[player][pos] & ~block;
        /* Add actual taking moves */
        moves |= pawn_takes[player][pos] & (state->occ_a[opponent[player]] | state->en_passant);
        /* Claim for pawns is only taking moves */
        state->claim[player] |= pawn_takes[player][pos] & ~state->occ_a[player];
      }
      break;
    case ROOK:
      moves = get_rook_moves(state, pos);
      break;
    case KNIGHT:
      moves = knight_moves[pos];
      break;
    case BISHOP:
      moves = get_bishop_moves(state, pos);
      break;
    case QUEEN:
      moves = get_bishop_moves(state, pos) | get_rook_moves(state, pos);
      break;
    case KING:
    /* Player info is required for castling checking */
      moves = get_king_moves(state, pos, player);
      break;
    default:
      moves = 0;
      break;
    }
    /* You can't take your own piece */
    moves &= ~state->occ_a[player];
      
    state->moves[index] = moves;
    if(piece_type[piece] != PAWN) {
      state->claim[player] |= moves;
    }
  }
  
  for(int8_t index = 0; index < N_PIECES; index++) {
    pos_t pos = state->piece_pos[index];      
    if(pos == NO_POS) {
      continue;
    }
    ASSERT(state->index_at[pos] == index);
    int piece = state->piece_at[pos];
    if(piece_type[piece] != KING) 
      continue;
    player_e player = piece_player[piece];
    bitboard_t moves = get_king_moves(state, state->piece_pos[index], player);
    /* You can't take your own piece */
    moves &= ~state->occ_a[player];
    state->moves[index] = moves;
    state->claim[player] |= moves;
  }
}

/* Initialises the module */
void init_moves(void)
{
  /* Position-indexed tables */
  for(pos_t pos = 0; pos < N_POS; pos++) {
    int mirror = (pos / 8) * 8 + (7 - pos % 8);
    shift_d[pos] = shift_c[mirror];
    mask_d[pos] = mask_c[mirror];
  }

  /* Table of file 0 occupancies */
  for(int i = 0; i < 256; i++) {
    occ_b2a[i] = 0;
    occ_c2a[i] = 0;
    occ_d2a[i] = 0;
    for(int bit = 0; bit < 8; bit++) {
      if(i & (1 << bit)) {
      occ_b2a[i] |= pos2mask[pos_a2b[bit]]; 
      occ_c2a[i] |= pos2mask[bit * 7];
      occ_d2a[i] |= pos2mask[bit * 9];
      }
    }
  }

  /*
   *  Sliding Moves
   */

  for(pos_t pos = 0; pos < 8; pos++) {
    unsigned int rank;
    for(rank = 0; rank < 256; rank++) {
      unsigned int l_mask, r_mask;
      unsigned int l_squares, r_squares, l_pieces, r_pieces;
      int l_first_occupied, r_first_occupied;

      /* 1. Set of all squares to the left of pos */
      l_squares = 0xff << (pos + 1);
      /* 2. Set of all pieces to the left of pos */
      l_pieces = l_squares & rank;
      if(l_pieces) {
	    /* 3. First occupied square to the left of pos (count trailing zeroes) */
	    l_first_occupied = ctz(l_pieces);
	    /* 4. Set of all squares to the left of the first occupied */
	    l_mask = 0xff << (l_first_occupied + 1);
      } else {
	    /* Hack because CTZ is undefined when there are no occupied squares */
	    l_mask = 0;
      }
		 
      /* Ditto right hand side, shift the other way and count leading zeroes  */
      r_squares = 0xff >> (8 - pos);
      r_pieces = r_squares & rank;
      if(r_pieces) {
	    /* Adjust the shift for the size of the word */
	    r_first_occupied = sizeof(unsigned int) * 8 - 1 - clz(r_pieces);
	    r_mask = 0xff >> (8 - r_first_occupied);
      } else {
	    r_mask = 0;
      }

      /* Combine left and right masks and negate to get set of all possible 
	     moves on the rank. */
      blocking[pos][rank] = ~(r_mask | l_mask);	  
    }
  }


  /* 
   *  Knight and King moves 
   */

  for(pos_t pos = 0; pos < N_POS; pos++) {
    bitboard_t pos_mask = pos2mask[pos];

    /* Possible knight moves: A-H
     * Possible king moves: I-P
     *      A   B
     *    H I J K C
     *      P * L
     *    G O N M D
     *      F   E      
     */

    /* BE, KLM */
    if(pos_mask & 0x7f7f7f7f7f7f7f7full) {
      knight_moves[pos] |= pos_mask >> 15;
      knight_moves[pos] |= pos_mask << 17;
      king_moves[pos] |= pos_mask << 1;
      king_moves[pos] |= pos_mask >> 7;
      king_moves[pos] |= pos_mask << 9;
    }
    /* CD */
    if(pos_mask & 0x3f3f3f3f3f3f3f3full) {
      knight_moves[pos] |= pos_mask >> 6;
      knight_moves[pos] |= pos_mask << 10;
    }
    /* AF, IOP */
    if(pos_mask & 0xfefefefefefefefeull) {
      knight_moves[pos] |= pos_mask >> 17;
      knight_moves[pos] |= pos_mask << 15;
      king_moves[pos] |= pos_mask >> 1;
      king_moves[pos] |= pos_mask << 7;
      king_moves[pos] |= pos_mask >> 9;
    }
    /* GH */
    if(pos_mask & 0xfcfcfcfcfcfcfcfcull) {
      knight_moves[pos] |= pos_mask >> 10;
      knight_moves[pos] |= pos_mask << 6;
    }
    /* JN */
    king_moves[pos] |= pos_mask >> 8;
    king_moves[pos] |= pos_mask << 8;
  }


  /*
   *  Pawn moves 
   */

  for(player_e player = 0; player < N_PLAYERS; player++) {
    for(pos_t pos = 0; pos < N_POS; pos++) {
      bitboard_t current, advance, jump, take;
      /* Mask for pawn */
      current = pos2mask[pos];
      advance = current;
      /* White and black advance in different directions */
      if(player == WHITE) {
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
      pawn_advances[player][pos] = advance | jump;
	  
      /* Taking moves */
      take = 0;
      /* If pawn is not on file 0, it can move to a lower file to take */
      if(current & 0x7f7f7f7f7f7f7f7full) {
      	take |= advance << 1;
      }  
      /* If pawn is not on file 7, it can move to a higher file to take */
      if(current & 0xfefefefefefefefeull) {
	      take |= advance >> 1;
      }
      /* Set of all possible taking moves */		 
      pawn_takes[player][pos] = take;
    }
  }
}