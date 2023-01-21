#include <stdint.h>
#include <stdio.h>

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
  return 0ull;
}

static inline int max(int a, int b) { return (a > b) ? a : b; }

int see_after_move(const struct position *position, enum square square,
                   enum piece moving_piece) {
  enum piece pieces[32];

  /* First victim */
  pieces[0] = see_piece_scores[piece_type[position->piece_at[square]]];

  int index = 1;
  enum player player = !position->turn;
  for (index = 1; index < 32; index++) {
    bitboard_t attackers = get_attacks(position, square, player);
    if (!attackers) break;

    int base = player * N_PIECE_T;
    bitboard_t lva = 0ull;
    enum piece start = PAWN;
    enum piece piece;
    for (piece = start; piece < KING; piece++) {
      bitboard_t lvas = position->a[base + piece] & attackers;
      if (lvas) {
        lva = take_next_bit_from(&lvas);
        attackers &= ~lva;
        break;
      } else {
        start++;
      }
    }

    if (piece == KING) break;

    pieces[index] = see_piece_scores[piece];
    player = !player;
  }

  while (--index) {
    // printf("%d %d\n", pieces[index], pieces[index - 1]);
    pieces[index - 1] = -max(-pieces[index - 1], pieces[index]);
  }
  return pieces[0];
}