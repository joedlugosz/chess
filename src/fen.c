
#include "chess.h"
#include "board.h"

/*
 *   Routines for FEN input and output
 *
 *   4.3 24/02/18 - Created 
 */

/* FEN piece letters */
const char piece_letter[N_PLANES + 1] = "prnbqkPRNBQK";

/* Indexes of pieces */
const int piece_index[N_PIECE_T * 2][8] = {
  {  0,  7, -1, -1, -1, -1, -1, -1 }, /* White has 2 rooks numbered 0 and 7 */
  {  1,  6, -1, -1, -1, -1, -1, -1 }, /* Knights */
  {  2,  5, -1, -1, -1, -1, -1, -1 }, /* Bishops */
  {  3, -1, -1, -1, -1, -1, -1, -1 }, /* Queen */
  {  4, -1, -1, -1, -1, -1, -1, -1 }, /* King */
  {  8,  9, 10, 11, 12, 13, 14, 15 }, /* Pawns */
  { 24, 31, -1, -1, -1, -1, -1, -1 }, /* Black rooks */
  { 25, 30, -1, -1, -1, -1, -1, -1 }, /* etc. */
  { 26, 29, -1, -1, -1, -1, -1, -1 },
  { 27, -1, -1, -1, -1, -1, -1, -1 },
  { 28, -1, -1, -1, -1, -1, -1, -1 },
  { 16, 17, 18, 19, 20, 21, 22, 23 }  
};

/* Loads a board state given in FEN, into state */
int load_fen(state_s *state, const char *placement, const char *active,
             const char *castling, const char *en_passant)
{
  /* Counters for number of each piece type already placed on the board */
  int count[N_PIECE_T * 2];
  /* Array representing pieces on the board, to be passed to setup_board() */
  int board[N_SQUARES];
  pos_t pos = 0;
  const char *ptr = placement;

  /* Start with empty board and zero counters */
  memset(board, EMPTY, sizeof(board));
  memset(count, 0, sizeof(count));

  while(*ptr) {
    if(*ptr == '/') {
      /* '/' marks the end of a rank, go to the start of the next one */
      pos = (pos / 8 + 1) * 8;
    } else if(*ptr >= '1' && *ptr <= '8') {
      /* Numeric input skips empty squares */
      pos += *ptr - '0';
    } else {
      /* Otherwise, lookup the piece descriptor from the list */
      int piece;
      for(piece = 0; piece < N_PLANES; piece++) {
        if(*ptr == piece_letter[piece]) {
          board[pos++] = piece;
          count[piece]++;
          break;
        }
      }
      if(piece == N_PLANES) {
        printf("Unrecognised FEN input: '%c'\n", *ptr);
        return 1;
      }
    }
    ptr++;
  }
  /* Success - write the new positions to state */
  setup_board(state, board);

  if(active[0] == 'w') {
    state->to_move = WHITE;
  } else if(active[0] == 'b') {
    state->to_move = BLACK;
  } else {
    printf("Unrecognised FEN input: '%s'\n", active);
  }

  return 0;
}

int get_fen(state_s *state, char *out, size_t outsize)
{
  int count = 0;
  char *ptr = out;
  for(pos_t pos = 0; pos < N_SQUARES; pos++) {
    int piece = state->piece_at[pos];
    if(piece == EMPTY) {
      count++;
    } else {
      if(count > 0) {
	*ptr++ = (char)count + '0';
      }
      *ptr++ = piece_letter[piece];
      count = 0;
    }
    if((pos % 8) == 7 && pos < 63) {
      *ptr++ = '/';
      count = 0;
    }
  }
  *ptr = 0;
  return 0;
}
