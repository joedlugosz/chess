#include "moves.h"
#include "position.h"

const int see_piece_scores[] = {1, 5, 3, 3, 9};

bitboard_t get_lva(const struct position *position, bitboard_t attackers) {
  int base = position->turn * N_PIECE_T;
  bitboard_t lva = 0ull;
  for (enum piece piece = PAWN; piece < KING; piece++) {
    bitboard_t lvas = position->a[base + piece] & attackers;
    if (lvas) {
      lva = take_next_bit_from(&lvas);
      return lva;
    }
  }
}

int see_after_move(const struct position *position, const struct move *move) {
  enum square square = move->to;
  enum piece moving_piece = move->piece;
  enum piece first_victim = piece_type[position->piece_at[square]];
  int score = see_piece_scores[first_victim];

  enum piece pieces[32];

  int index;
  enum player player = !position->turn;
  for (;;) {
    bitboard_t attackers = get_attacks(position, square, player);
    if (!attackers) break;

    int base = player * N_PIECE_T;
    bitboard_t lva = 0ull;
    enum piece piece;
    for (piece = PAWN; piece < KING; piece++) {
      bitboard_t lvas = position->a[base + piece] & attackers;
      if (lvas) {
        lva = take_next_bit_from(&lvas);
        attackers &= ~lva;
        break;
      }
    }

    if (piece == KING) break;

    pieces[index] = see_piece_scores[piece];
    index++;
    player = !player;
  }

  while (--index) {
    printf("%d ", pieces[index]);
  }
  printf("\n");

  return 0;
}