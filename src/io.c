/*
 *   Text IO formatting and parsing
 */

#include "compiler.h"
#include "state.h"
#include "search.h"
#include "os.h"
#include "log.h"

#include <ctype.h>
#include <stdarg.h>
#include <time.h>

#if (TERM_UNICODE)
# define FULL_SQUARE "\u2b24"
# define EMPTY_SQUARE "\u25ef"
# define SELECT_SQUARE "\u25a6"
#else
# define FULL_SQUARE "X"
# define EMPTY_SQUARE "'"
# define SELECT_SQUARE "*"
#endif

const char piece_text_ascii[N_PLANES][6] = {
  "p ", "R ", "N ", "B ", "Q ", "K ",
  "p*", "R*", "N*", "B*", "Q*", "K*"
};
static const char piece_letter[] = "prnbq";

#if(TERM_UNICODE)
const char piece_text_unicode[N_PLANES][6] = {
  "\u265f ", "\u265c ", "\u265e ", "\u265d ", "\u265b ", "\u265a ",
  "\u2659 ", "\u2656 ", "\u2658 ", "\u2657 ", "\u2655 ", "\u2654 "
};
# define PIECE_TEXT_TERM piece_text_unicode
# define PIECE_TEXT_FILE piece_text_ascii
#else
# define PIECE_TEXT_TERM piece_text_ascii
#endif
#define PIECE_TEXT_FILE piece_text_ascii

const char player_text[N_PLAYERS][6] = {
  "WHITE", "BLACK"
};

/*
 *  Instruction encoding and decoding
 */
int parse_square(const char *buf, square_e *square)
{
  const char *ptr = buf;
  *square = NO_SQUARE;
  if(*ptr == 0) return 1;
  if(!isalpha(*ptr)) return 1;
  *square = (square_e)(tolower(*ptr) - 'a');
  if(*square > 7) return 1;
  ptr++;
  if(*ptr == 0) return 1;
  if(!isdigit(*ptr)) return 1;
  if(*ptr > '8') return 1;
  *square += 8 * (square_e)(*ptr - '1');
  ptr++;
  return 0;
}

int parse_move(const char *buf, move_s *move)
{
  if(parse_square(buf, &move->from)) return 1;
  if(parse_square(buf+2, &move->to)) return 1;
  if(buf[4] == 0) {
    move->promotion = 0;
    return 0;
  }
  for(piece_e piece = ROOK; piece <= QUEEN; piece++) {
    if(tolower(buf[4]) == piece_letter[piece]) {
      move->promotion = piece;
      return 0;
    }
  }
  return 1;
}

int format_square(char *buf, square_e square)
{
  if(square < 0 || square >= N_SQUARES) {
    buf[0] = 0;
    return 1;
  }
  buf[0] = (char)(square % 8) + 'a';
  buf[1] = (char)(square / 8) + '1';
  buf[2] = 0;
  return 0;
}

int format_move(char *buf, move_s *move, int bare)
{
  char *ptr = buf;
  if(format_square(ptr, move->from)) return 1;
  ptr += 2;
  if(!bare && (move->result & CAPTURED))
    *ptr++ = 'x';
  if(format_square(ptr, move->to)) return 1;
  ptr += 2;
  if(move->promotion > PAWN) {
    *ptr++ = piece_letter[move->promotion];
  }
  if(!bare) {
    if(move->result & CHECK) *ptr++ = '+';
    if(move->result & MATE) *ptr++ = '#';
  }
  return 0;
}

void print_thought_moves(FILE *f, int depth, move_s moves[])
{
  char buf[6];
  int i;
  for(i = 0; i <= depth; i++) {
    format_move(buf, &moves[i], 0);
    fprintf(f, "%s ", buf);
  }
}

/*
 *  Board printing
 */
void print_board(FILE *f, state_s *state, bitboard_t mask1, bitboard_t mask2)
{
  int rank, file;
  int term;

  if(!f) return; 
  term = is_terminal(f);

  fprintf(f, "\n");
  for(rank = 7; rank >= 0; rank--) {
    fprintf(f, "%s", "    ");
#if(ORDER_BINARY)
    for(file = 7; file >= 0; file--) 
#else
      for(file = 0; file <= 7; file++) 
#endif
	{
	  int piece = state->piece_at[rank * 8 + file];
	  /* Print square colours if a terminal */
	  if(term) {
	    if(mask1 & square2bit[rank * 8 + file]) {
	      set_console_hilight1();
	    } else if(mask2 & square2bit[rank * 8 + file]) {
	      set_console_hilight2();
	    } else if((rank + file) & 1) {
	      set_console_white_square();
	    } else {
	      set_console_black_square();
	    }
	  }
	  if(piece != EMPTY) {
	    if(term) {
	      if(piece_player[piece] == WHITE) {
		set_console_white_piece();
	      } else {
		set_console_black_piece();
	      }
	    }
	    fprintf(f, "%s", PIECE_TEXT_TERM[piece]);
	  } else {
	    fprintf(f, "%s", "  ");
	  }
	}
    /* End each line with black */
    if(term) {
      set_console_black_square();
      set_console_black_piece();
    }
    fprintf(f, "\n");
  }
  fprintf(f, "\n");
}

/*
 *  Tokeniser
 */
#define INPUT_BUF_SIZE 1024
char input_buf[INPUT_BUF_SIZE];

/* Get text from stdin up to the next whitespace */
const char *get_input(void) {
  char *ptr = input_buf;
  char *end = input_buf + sizeof(input_buf) - 1;
  /* Read input until first non whitespace character */
  while (isspace(*ptr = fgetc(stdin)));
  /* Read input until first whitespace character */
  while (++ptr < end) {
    *ptr = fgetc(stdin);
    if (isspace(*ptr)) break;
  }
  *ptr = 0;
  return input_buf;
}

/* Get text from stdin up to a delimiter char */
const char *get_delim(char delim) {
  char *ptr = input_buf;
  while (ptr < input_buf + sizeof(input_buf) - 1) {
    *ptr = fgetc(stdin);
    if (*ptr == delim) {
      *ptr = 0;
      return input_buf;
    }
    ptr++;
  }
  *ptr = 0;
  return input_buf;
}
