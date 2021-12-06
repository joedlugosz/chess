
/*
 *   Routines for FEN input and output
 */

#include "board.h"
#include "io.h"

/* FEN piece letters */
static const char piece_letter[N_PLANES + 1] = "PRNBQKprnbqk";

enum { N_CASTLE_RIGHTS_MASKS = 4 };

struct castle_rights_entry {
  char c;
  plane_t mask;
};

static const struct castle_rights_entry 
  castling_rights_letter[N_CASTLE_RIGHTS_MASKS] = {
  { 'K', WHITE_KINGSIDE },
  { 'Q', WHITE_QUEENSIDE },
  { 'k', BLACK_KINGSIDE },  
  { 'q', BLACK_QUEENSIDE }
};

/* Loads a board state given in FEN, into state */
int load_fen( state_s *state, 
              const char *placement_text, 
              const char *active_player_text,
              const char *castling_text, 
              const char *en_passant_text)
{
  /* Counters for number of each piece type already placed on the board */
  int count[N_PIECE_T * 2];
  /* Array representing pieces on the board, to be passed to setup_board() */
  int board[N_POS];
  int file = 0;
  int rank = 7;
  const char *ptr = placement_text;
  const char *error_text;

  /* Start with empty board and zero counters */
  memset(board, EMPTY, sizeof(board));
  memset(count, 0, sizeof(count));

  error_text = placement_text;
  while(*ptr) {
    if(*ptr == '/') {
      /* '/' marks the end of a rank, go to the start file of the next one */
      if(file < 8) {
        printf("FEN: Not enough rank input\n");
        goto error;
      }
      file = 0;
      rank--;
    } else if(*ptr >= '1' && *ptr <= '8') {
      /* Numeric input skips empty squares */
      file += *ptr - '0';
      if(file > 8) {
        printf("FEN: Too much rank input\n");
        goto error;
      }
    } else {
      /* Otherwise, lookup the piece descriptor from the list */
      int piece;
      for(piece = 0; piece < N_PLANES; piece++) {
        if(*ptr == piece_letter[piece]) {
  	      /* Set index, increment counters */
          board[rank*8+file] = piece;
          count[piece]++;
          break;
        }
      }
      /* Unrecognised characters */
      if(piece == N_PLANES) {
	      printf("FEN: Unrecognised piece\n");
	      goto error;
      }
      file++;
      if(file > 8) {
        printf("FEN: Too much rank input\n");
        goto error;
      }
    }
  
    /* Too much input */
    if(file > 8 || rank < 0) {
      printf("FEN: Too much board input\n");
	    goto error;
	  }
    ptr++;
  }
  /* Not enough input */
  if(rank > 0) {
    printf("FEN: Not enough board input\n");
    goto error;
  }
  
  player_e to_move;
  error_text = active_player_text;
  if(active_player_text[0] == 'w' && active_player_text[1] == 0) {
    to_move = WHITE;
  } else if(active_player_text[0] == 'b' && active_player_text[1] == 0) {
    to_move = BLACK;
  } else {
    printf("Unrecognised active player input\n");
    goto error;
  }
  
  ptr = castling_text;
  error_text = castling_text;
  plane_t castling_rights = 0;
  while(*ptr) {
    for(int i = 0; i < N_CASTLE_RIGHTS_MASKS; i++) {
      if(*ptr == castling_rights_letter[i].c) {
        castling_rights |= castling_rights_letter[i].mask;
      }
    }
    ptr++;
  }
  if(castling_rights == 0 && *castling_text != '-') {
    printf("FEN: No castling flags found\n");
    goto error;
  }

  ptr = en_passant_text;
  pos_t en_passant;
  if(*ptr == '-') {
    en_passant = NO_POS;
  } else {
    if(parse_pos(en_passant_text, &en_passant)) {
      printf("FEN: Invalid en-passant input\n");
      goto error;
    }
  }
  /* Success - write the new positions to state */
  setup_board(state, board, to_move, castling_rights, en_passant);
  return 0;
 error:
  printf("\nFEN input: %s", error_text);
  printf("\n         : %*c^\n", (int)(ptr-error_text), ' ');
  return 1;
}

int get_fen(const state_s *state, char *out, size_t outsize)
{
  int empty_file_count = 0;
  char *ptr = out;
  for(int rank = 7; rank >= 0; rank--) {
    for(int file = 0; file < 8; file++) {
      int piece = state->piece_at[rank*8+file];
      if(piece == EMPTY) {
        empty_file_count++;
      } else {
        /* Piece */
        if(empty_file_count > 0) {
          *ptr++ = (char)empty_file_count + '0';
          empty_file_count = 0;
        }
        *ptr++ = piece_letter[piece];
      }
      /* End of rank */
      if(file == 7) {
        if(empty_file_count > 0) {
          *ptr++ = (char)empty_file_count + '0';
          empty_file_count = 0;
        }
        if(rank > 0) {
          *ptr++ = '/';
        }
      }
    }
  }
  *ptr++ = ' ';
  /* Moving player */
  *ptr++ = (state->to_move == WHITE) ? 'w' : 'b';
  *ptr++ = ' ';
  /* Castling rights */
  for(int i = 0; i < N_CASTLE_RIGHTS_MASKS; i++) {
    if(state->castling_rights & castling_rights_letter[i].mask) {
      *ptr++ = castling_rights_letter[i].c;
    }
  }
  if(!state->castling_rights) *ptr++ = '-';
  *ptr++ = ' ';
  /* En passant */
  if(state->en_passant == NO_POS) {
    *ptr++ = '-';
  } else {
    format_pos(ptr, state->en_passant);
    ptr += 2;
  }
  //*ptr++ = ' ';
  //*ptr++ = '0';
  //*ptr++ = ' ';
  //*ptr++ = '0';
  *ptr = 0;
  return 0;
}
