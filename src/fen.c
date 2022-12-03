
/*
 *  Forsyth-Edwards Notation input and output
 */

#include <stdio.h>

#include "io.h"
#include "position.h"

/* FEN piece letters */
static const char piece_letter[N_PLANES + 1] = "PRNBQKprnbqk";

enum { N_CASTLE_RIGHTS_MASKS = 4 };

/* Mapping between a FEN castling rights letter and a bit in the castling
   rights bitmask */
struct castle_rights_entry {
  char c;
  castle_rights_t bits;
};

/* Mapping of FEN castling rights letters to castling rights bits */
static const struct castle_rights_entry
    castling_rights_letter[N_CASTLE_RIGHTS_MASKS] = {{'K', WHITE_KINGSIDE},
                                                     {'Q', WHITE_QUEENSIDE},
                                                     {'k', BLACK_KINGSIDE},
                                                     {'q', BLACK_QUEENSIDE}};

/* Load a board position given in FEN, into position */
int load_fen(struct position *position, const char *placement_text,
             const char *active_player_text, const char *castling_text,
             const char *en_passant_text, const char *halfmove_text,
             const char *fullmove_text) {
  /* Counters for number of each piece type already placed on the board */
  int count[N_PIECE_T * 2];
  /* Array representing pieces on the board, to be passed to setup_board() */
  enum piece board[N_SQUARES];
  int file = 0;
  int rank = 7;
  const char *ptr = placement_text;
  const char *error_text;

  /* Start with empty board and zero counters */
  memset(board, EMPTY, sizeof(board));
  memset(count, 0, sizeof(count));

  /* Placement */
  error_text = placement_text;
  while (*ptr) {
    if (*ptr == '/') {
      /* '/' marks the end of a rank, go to the start file of the next one */
      if (file < 8) {
        printf("FEN: Not enough rank input\n");
        goto error;
      }
      file = 0;
      rank--;
    } else if (*ptr >= '1' && *ptr <= '8') {
      /* Numeric input skips empty squares */
      file += *ptr - '0';
      if (file > 8) {
        printf("FEN: Too much rank input\n");
        goto error;
      }
    } else {
      /* Otherwise, lookup the piece descriptor from the list */
      int piece;
      for (piece = 0; piece < N_PLANES; piece++) {
        if (*ptr == piece_letter[piece]) {
          /* Set index, increment counters */
          board[rank * 8 + file] = (enum piece)piece;
          count[piece]++;
          break;
        }
      }
      if (piece == N_PLANES) {
        printf("FEN: Unrecognised piece\n");
        goto error;
      }
      file++;
      if (file > 8) {
        printf("FEN: Too much rank input\n");
        goto error;
      }
    }

    if (file > 8 || rank < 0) {
      printf("FEN: Too much board input\n");
      goto error;
    }
    ptr++;
  }
  if (rank > 0) {
    printf("FEN: Not enough board input\n");
    goto error;
  }

  /* Turn */
  enum player turn;
  error_text = active_player_text;
  if (active_player_text[0] == 'w' && active_player_text[1] == 0) {
    turn = WHITE;
  } else if (active_player_text[0] == 'b' && active_player_text[1] == 0) {
    turn = BLACK;
  } else {
    printf("Unrecognised active player input\n");
    goto error;
  }

  /* Castling rights */
  ptr = castling_text;
  error_text = castling_text;
  castle_rights_t castling_rights = 0;
  while (*ptr) {
    for (int i = 0; i < N_CASTLE_RIGHTS_MASKS; i++) {
      if (*ptr == castling_rights_letter[i].c) {
        castling_rights |= castling_rights_letter[i].bits;
      }
    }
    ptr++;
  }
  if (castling_rights == 0 && *castling_text != '-') {
    printf("FEN: No castling flags found\n");
    goto error;
  }

  /* En passant */
  ptr = en_passant_text;
  bitboard_t en_passant;
  if (*ptr == '-') {
    en_passant = 0;
  } else {
    enum square ep_square;
    if (parse_square(en_passant_text, &ep_square)) {
      printf("FEN: Invalid en-passant input\n");
      goto error;
    }
    en_passant = square2bit[ep_square];
  }

  /* Halfmove and fullmove */
  int halfmove;
  if (sscanf(halfmove_text, "%d", &halfmove) != 1) {
    printf("FEN: Invalid halfmove clock input\n");
    goto error;
  }
  int fullmove;
  if (sscanf(fullmove_text, "%d", &fullmove) != 1) {
    printf("FEN: Invalid move number input\n");
    goto error;
  }

  /* Success - write the new positions to position */
  setup_board(position, board, turn, castling_rights, en_passant, halfmove,
              fullmove);
  return 0;

  /* Input error - display location */
error:
  printf("\nFEN input: %s", error_text);
  printf("\n         : %*c^\n", (int)(ptr - error_text), ' ');
  return 1;
}

/* Format a FEN string from a position */
int get_fen(const struct position *position, char *out, size_t outsize) {
  /* Placement */
  int empty_file_count = 0;
  char *ptr = out;
  for (int rank = 7; rank >= 0; rank--) {
    for (int file = 0; file < 8; file++) {
      int piece = position->piece_at[rank * 8 + file];
      if (piece == EMPTY) {
        empty_file_count++;
      } else {
        /* Piece */
        if (empty_file_count > 0) {
          *ptr++ = (char)empty_file_count + '0';
          empty_file_count = 0;
        }
        *ptr++ = piece_letter[piece];
      }
      /* End of rank */
      if (file == 7) {
        if (empty_file_count > 0) {
          *ptr++ = (char)empty_file_count + '0';
          empty_file_count = 0;
        }
        if (rank > 0) {
          *ptr++ = '/';
        }
      }
    }
  }
  *ptr++ = ' ';

  /* Turn */
  *ptr++ = (position->turn == WHITE) ? 'w' : 'b';
  *ptr++ = ' ';

  /* Castling rights */
  for (int i = 0; i < N_CASTLE_RIGHTS_MASKS; i++) {
    if (position->castling_rights & castling_rights_letter[i].bits) {
      *ptr++ = castling_rights_letter[i].c;
    }
  }
  if (!position->castling_rights) *ptr++ = '-';
  *ptr++ = ' ';

  /* En passant */
  if (position->en_passant == 0) {
    *ptr++ = '-';
  } else {
    format_square(ptr, bit2square(position->en_passant));
    ptr += 2;
  }

  /* Halfmove and fullmove counts */
  sprintf(ptr, " %d %d", position->halfmove, position->fullmove);
  return 0;
}
