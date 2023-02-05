#include <stdint.h>
#include <stdio.h>

#include "fen.h"
#include "io.h"
#include "moves.h"
#include "position.h"

const int see_piece_scores[] = {1, 5, 3, 3, 9, 20};
const char names[] = {'p', 'R', 'N', 'B', 'Q', 'K'};

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

int see_after_move(const struct position *position, enum square from,
                   enum square to, enum piece attacker) {
  enum player player = position->turn;

  bitboard_t rook_sliders = position->a[ROOK] | position->a[QUEEN] |
                            position->a[ROOK + N_PIECE_T] |
                            position->a[QUEEN + N_PIECE_T];
  bitboard_t bishop_sliders = position->a[BISHOP] | position->a[QUEEN] |
                              position->a[BISHOP + N_PIECE_T] |
                              position->a[QUEEN + N_PIECE_T];
  bitboard_t occupancy = position->total_a;

  // print_board(position, 0, 0);

  int scores[32];
  /* First victim */
  enum piece victim = piece_type[position->piece_at[to]];
  scores[0] = see_piece_scores[victim];
  // printf("%d: player %d, %c attacks %c >> %d\n", 0, player, names[attacker],
  //        names[victim], scores[0]);

  player = !player;

  bitboard_t attackers =
      (get_attacks(position, to, player) | get_attacks(position, to, !player));

  /* Remove the original position of the first attacker */
  occupancy &= ~square2bit[from];

  /* A sliding attacker may have uncover sliding attacker behind it */
  if (attacker == PAWN || attacker == BISHOP || attacker == QUEEN)
    attackers |= get_bishop_moves(occupancy, to) & bishop_sliders;
  if (attacker == ROOK || attacker == QUEEN)
    attackers |= get_rook_moves(occupancy, to) & rook_sliders;

  /* First attacker becomes second potential victim */
  victim = attacker;
  scores[1] = see_piece_scores[victim] - scores[0];
  // printf("%d: player %d, find pieces to attack %c >> %d\n", 1, player,
  //        names[victim], scores[1]);

  /* Find attackers in ascending order for each side */
  int index;
  for (index = 2; index < 32; index++) {
    /* No more attackers */
    if (!(occupancy & attackers & position->player_a[player])) break;
    int base = player * N_PIECE_T;

    /* Find least valuable attacker */
    for (attacker = PAWN; attacker <= KING; attacker++) {
      bitboard_t lv_attackers =
          occupancy & attackers & position->a[base + attacker];
      if (lv_attackers) {
        bitboard_t lva = take_next_bit_from(&lv_attackers);
        occupancy &= ~lva;
        /* A sliding attacker may have uncover sliding attacker behind it */
        if (attacker == PAWN || attacker == BISHOP || attacker == QUEEN)
          attackers |= get_bishop_moves(occupancy, to) & bishop_sliders;
        if (attacker == ROOK || attacker == QUEEN)
          attackers |= get_rook_moves(occupancy, to) & rook_sliders;
        break;
      }
    }

    if (attacker > KING) break;

    scores[index] = see_piece_scores[attacker] - scores[index - 1];
    // printf("%d: player %d, %c attacks %c = %d\n", index, player,
    //        names[attacker], names[victim], scores[index]);
    player = !player;
    victim = attacker;
  }

  /* The last piece in the array is the last piece standing, so it is not
     counted for SEE. */
  index--;

  // for (int i = 0; i < index; i++) {
  //   printf("%d: %d\n", i, scores[i] * ((i % 2) ? -1 : 1));
  // }
  while (--index) {
    // printf("%d: s[n]=%d s[n-1]=%d\n", index, scores[index], scores[index -
    // 1]);
    scores[index - 1] = -max(-scores[index - 1], scores[index]);
    // printf("%d: s[n-1] <- %d\n", index, scores[index - 1]);
  }
  // printf("SEE %d\n", scores[0]);
  return scores[0];
}
