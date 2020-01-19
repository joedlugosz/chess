/*
 *  game.c
 *
 *  Definition of the game state and FIDE rules.
 *
 *  Limitations:
 *   * No en passant taking
 *   * No under-promotion
 *
 *  Revisions:
 *   0.1  09/04/16  Started
 *   4.0            Pre-computed values, changed piece_at from piece type to index
 *   4.4  28/02/18  Changed most piece_index stuff back to piece_type, parallel piece_at and index_at arrays
 *                  Fixed bug preventing castling
 *   5.0            Added assertion for attack check on own pieces
 *                  Fixed attack checking during castling
 *   5.1            En passant, underpromotion, 
 *                  Changed searching for slider attacks,
 *                  Replaced moved pieces plane with castling rights flags
 */

#include "chess.h"
#include "board.h"
#include "log.h"

typedef unsigned char rank_t;

enum {
  N_HASH_VALUES = N_PIECE_T * N_POS  
};

hash_t hash_values[N_HASH_VALUES];

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

/* These are reflections in the vertical axis of their C-pos counterparts 
   which are calculated at run-time */
pos_t pos_a2d[N_POS];
rank_t shift_d[N_POS];
rank_t mask_d[N_POS];

/* Rank blocking table for Rook, Bishop and Queen moves */
unsigned int blocking[8][256];

/* For B,C,D-stacks, masks taking rank occupancy back into a vertical or 
   diagonal in A-plane */
plane_t occ_b2a[256], occ_c2a[256], occ_d2a[256];

/* Knight and king moves */
plane_t knight_moves[N_POS], king_moves[N_POS];
/* Pawn advances and takes */
plane_t pawn_advances[N_PLAYERS][N_POS], pawn_takes[N_PLAYERS][N_POS];
/* Castling */
const pos_t rook_start_pos[N_PLAYERS][2] = { { 0, 7 }, { 56, 63 } };
const pos_t king_start_pos[N_PLAYERS] = { 4, 60 };
const plane_t king_castle_slides[N_PLAYERS][2] = { { 0x0cull, 0x60ull }, { 0x0cull << 56, 0x60ull << 56 } };
const plane_t rook_castle_slides[N_PLAYERS][2] = { { 0x0eull, 0x60ull }, { 0x0eull << 56, 0x60ull << 56 } };
const plane_t castle_moves[N_PLAYERS][2] = { { 0x01ull, 0x80ull }, { 0x01ull << 56, 0x80ull << 56 } };
const plane_t castle_destinations[N_PLAYERS][2] = { { 0x04ull, 0x40ull }, { 0x04ull << 56, 0x40ull << 56 } };
const castle_rights_t castle_rights[N_PLAYERS][N_BOARDSIDE] = {
  { WHITE_QUEENSIDE, WHITE_KINGSIDE, WHITE_BOTHSIDES },
  { BLACK_QUEENSIDE, BLACK_KINGSIDE, BLACK_BOTHSIDES }
};


/* Starting positions encoded as A-stack */
const plane_t starting_a[N_PLANES] = {
  /* White
     Black */
  /* Pawns     Rooks        Bishops      Knights      Queen        King       */
  0xffull<<8,  0x81ull,     0x42ull,     0x24ull,     0x08ull,     0x10ull,
  0xffull<<48, 0x81ull<<56, 0x42ull<<56, 0x24ull<<56, 0x08ull<<56, 0x10ull<<56
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

static void calculate_moves(state_s *state);


/* Returns the valid rook moves for a given position, taking into account 
   blockages by other pieces.  Also returns moves where rooks can take their 
   own side's pieces (these are filtered out elsewhere). */
plane_t get_rook_moves(state_s *state, pos_t a_pos)
{
  /* A-stack */
  /* Shift required to shift the rank containing pos down to rank zero */
  int a_shift = a_pos & ~7ul;
  /* Position within that rank */
  int a_file = a_pos & 7ul;
  /* Piece occupancy of the rank in the A-stack */
  rank_t a_occ = (rank_t)((state->total_a >> a_shift) & 0xfful);
  /* Permitted moves within the rank */
  rank_t a_moves = blocking[a_file][a_occ];
  /* A-mask of permitted horizontal moves */
  plane_t h_mask = (plane_t)a_moves << a_shift;  
  /* B-stack */
  /* B-position */
  int b_pos = pos_a2b[a_pos];
  /* Shift required to shift the B-rank containing pos down to rank zero */
  int b_shift = b_pos & ~7ul;
  int b_file = b_pos & 7ul;
  /* Piece occupancy of the rank in the B-stack */
  rank_t b_occ = (rank_t)((state->total_b >> b_shift) & 0xfful);
  /* Permitted moves within the rank */
  rank_t b_moves = blocking[b_file][b_occ];
  /* A-mask (note) of permitted vertical moves in file 0 */
  plane_t b_mask = occ_b2a[b_moves];
  /* Shift A-mask to correct file position */
  plane_t v_mask = b_mask << a_file;
  /* Or the horizontal and vertical masks */
  return (h_mask | v_mask); 
}

/* Returns the valid bishop moves for a given position, taking into account 
   blockages by other pieces.  Also returns moves where bishops can take their
   own side's pieces (these are filtered out elsewhere). */
plane_t get_bishop_moves(state_s *state, pos_t a_pos)
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
  plane_t c_ret = occ_c2a[c_moves] << shift_c2a[a_pos];
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
  plane_t d_ret = occ_d2a[d_moves] << shift_d2a[a_pos];

  return (c_ret | d_ret); 
}

/* Returns the valid king moves for a given position, including king 
   destinations for castling if possible */
plane_t get_king_moves(state_s *state, pos_t from, player_e player)
{
  /* Non-castling king moves are taken from the array.  Any squares which are attacked by the 
   * opponent are removed */
  plane_t moves = king_moves[from] & ~state->claim[opponent[player]];

  /* Castling */
  /* TODO: This could probably be a lot better */
  /* The king must not have moved already - otherwise return non-castling moves */
  if(state->moved & starting_a[player * N_PIECE_T + KING]) {
    return moves;
  }
  /* There must be rooks that have not moved - otherwise return non-castling moves */
  if((state->moved & starting_a[player * N_PIECE_T + ROOK])
     == starting_a[player * N_PIECE_T + ROOK]) {
    return moves;
  }
  /* The king must not be under attack - otherwise return non-castling moves */
  if(get_attacks(state, from, opponent[player])) {
    return moves;
  }
  /* Get all pieces in the board which might block castling */
  plane_t all = state->total_a;
  /* For each side, left and right */
  for(int side = 0; side < 2; side++) {
    /* Can't castle if this side's rook has moved */
    if(state->moved & castle_moves[player][side]) {
      continue;
    }
    /* All squares that the king will slide through */
    plane_t king_slides = king_castle_slides[player][side];
    plane_t rook_slides = rook_castle_slides[player][side];
    /* Are there any pieces in the way of the rook sliding? */
    plane_t blocked = all & rook_slides;
    /* If so, blocked is nonzero, and can't castle on this side */
    if(!blocked) {
      /* For each slide square, mark any attacked square that the king
       * will move through as blocked */
      while(king_slides) {
        plane_t mask = next_bit_from(&king_slides);
        if(get_attacks(state, mask2pos(mask), opponent[player])) {
          blocked |= mask;
        }
      }
    }
    /* If there are no blocked or attacked squares preventing castling, 
     * add the destination as a valid move */
    if(!blocked) {
      moves |= castle_destinations[player][side];
    }
  }
  return moves;
}

/* Returns the set of all pieces that can attack a given position */
plane_t get_attacks(state_s *state, pos_t target, player_e attacking)
{
  plane_t attacks = 0;
  plane_t target_mask = pos2mask[target];
  int base = attacking * N_PIECE_T;
  /* Check not trying to attack own piece */
  ASSERT(piece_player[(int)state->piece_at[target]] != attacking);
  /* Note - viewed as if the non-attacking player is attacking */
  attacks = pawn_takes[opponent[attacking]][target] & state->a[base + PAWN];
  attacks |= knight_moves[target] & state->a[base + KNIGHT];
  attacks |= king_moves[target] & state->a[base + KING];
  plane_t sliders = state->a[base + ROOK] | state->a[base + BISHOP] | state->a[base + QUEEN];
  while(sliders) {
    plane_t attacker = next_bit_from(&sliders);
    if(target_mask & state->moves[(int)state->index_at[mask2pos(attacker)]]) attacks |= attacker;
  }
  return attacks;
}

/* Returns the set of all positions that a piece can move to, given the
   position of the moving piece. */
static void calculate_moves(state_s *state)
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
    plane_t moves;
    /* Get moves for the piece including taking moves for all pieces */
    switch(piece_type[piece]) {	
    case PAWN:
      {
        /* Pawns are blocked from moving ahead by any piece */
        plane_t block = state->total_a;
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
    plane_t moves = get_king_moves(state, state->piece_pos[index], player);
    /* You can't take your own piece */
    moves &= ~state->occ_a[player];
    state->moves[index] = moves;
    state->claim[player] |= moves;
  }
}

static inline void add_piece(state_s *state, pos_t pos, piece_e piece, int index)
{
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
  state->hash ^= hash_values[pos * piece];
}

static inline void remove_piece(state_s *state, pos_t pos)
{
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
  state->hash ^= hash_values[pos * piece];
}

static inline void clear_rook_castling_rights(state_s *state, pos_t pos)
{
  player_e victim_player = piece_player[state->piece_at[pos]];
  for(int side = 0; side < 2; side++) {
    if(pos == rook_start_pos[victim_player][side]) {
      state->castle_rights &= ~castle_rights[victim_player][side];
    }
  }
}

static inline void clear_king_castling_rights(state_s *state, player_e player)
{
  state->castle_rights &= ~castle_rights[player][BOTHSIDES];
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
  plane_t bb_to = pos2mask[move->to];
  int8_t victim_piece = state->piece_at[move->to];
  if(victim_piece != EMPTY) {
    ASSERT(piece_type[victim_piece] != KING);
    ASSERT(piece_player[victim_piece] != state->to_move);
    remove_piece(state, move->to);
    move->result |= CAPTURED;
    /* If a rook has been captured, set moved flag to prevent castling */
    if(piece_type[victim_piece] == ROOK) {
      clear_rook_castling_rights(state, move->to);
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
    clear_rook_castling_rights(state, move->from);
    break;
  case PAWN:
    /* If pawn has been promoted */
    if(bb_to & (0xffull << 56 | 0xffull)) {
      ASSERT(move->promotion > PAWN);
      remove_piece(state, move->to);
      add_piece(state, move->to, moving_piece + move->promotion - PAWN,
        moving_index);
      move->result |= PROMOTED;
    }
    /* If pawn has been taken en-passant */
    if(bb_to & state->en_passant) {
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
  state->en_passant = 0;
  /* If pawn has jumped, set en passant square */
  if(piece_type[moving_piece] == PAWN) {
    if(move->from - move->to == 16) {
      state->en_passant = pos2mask[move->from - 8];
    }
    if(move->from - move->to == -16) {
      state->en_passant = pos2mask[move->from + 8];
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
  player_e to_move, castle_rights_t castle_rights)
{
  memset(state, 0, sizeof(*state));
  state->to_move = to_move;
  state->castle_rights = castle_rights;

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
  setup_board(state, start_pieces, WHITE, ALL_CASTLE_RIGHTS);
}

/* Initialises the module */
void init_board(void)
{
  int i, mirror, bit;
  player_e player;
  plane_t pos_mask;
  pos_t pos;


  /* 
   *  Position-indexed tables 
   */
  for(pos = 0; pos < N_POS; pos++) {
    /* Position to mask */
    pos2mask[pos] = 1ull << pos;
    /* Calculate A->B */
    pos_a2b[pos] = (pos / 8) + (pos % 8) * 8;
    /* Calculate mirror in Files */
    mirror = (pos / 8) * 8 + (7 - pos % 8);
    /* Calculate A->D as mposrror of A->C */
    pos_a2d[pos] = pos_a2c[mirror];
    /* Shift and mask for D are mirrors of C */
    shift_d[pos] = shift_c[mirror];
    mask_d[pos] = mask_c[mirror];
  }


  /* 
   *  Rank-indexed tables 
   */

  for(i = 0; i < 256; i++) {
    /* Table of file 0 occupancies */
    occ_b2a[i] = 0;
    occ_c2a[i] = 0;
    occ_d2a[i] = 0;
    for(bit = 0; bit < 8; bit++) {
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

  for(pos = 0; pos < 8; pos++) {
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

  for(pos = 0; pos < N_POS; pos++) {
    pos_mask = pos2mask[pos];

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

  for(player = 0; player < N_PLAYERS; player++) {
    for(pos = 0; pos < N_POS; pos++) {
      plane_t current, advance, jump, take;
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

//  init_hash(hash, N_HASH_VALUES);
}
