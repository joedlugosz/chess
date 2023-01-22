#include <stdint.h>
#include <stdio.h>

#include "fen.h"
#include "moves.h"
#include "position.h"

const int see_piece_scores[] = {1, 5, 3, 3, 9};
const char names[] = {'p', 'R', 'N', 'B', 'Q'};

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
  int scores[32];
  int index = 0;

  enum player player = position->turn;

  /* First victim */
  enum piece victim = piece_type[position->piece_at[to]];
  scores[0] = see_piece_scores[victim];
  // printf("%d: player %d, %c attacks %c >> %d\n", index, player,
  // names[attacker],
  //        names[victim], scores[0]);

  player = !player;

  /* First attacker becomes second potential victim */
  victim = attacker;
  scores[1] = see_piece_scores[victim] - scores[0];
  // printf("%d: player %d, find pieces to attack %c >> %d\n", index, player,
  //        names[victim], scores[1]);

  bitboard_t attackers =
      (get_attacks(position, to, player) | get_attacks(position, to, !player)) &
      ~square2bit[from];

  for (index = 2; index < 32; index++) {
    // printf("index %d  attackers %llx \n", index, attackers);
    bitboard_t my_attackers = attackers & position->player_a[player];
    if (!my_attackers) break;

    int base = player * N_PIECE_T;
    bitboard_t lva = 0ull;
    enum piece start = PAWN;
    for (attacker = start; attacker < KING; attacker++) {
      bitboard_t lvas = position->a[base + attacker] & my_attackers;
      if (lvas) {
        lva = take_next_bit_from(&lvas);
        attackers &= ~lva;
        break;
      } else {
        // start++;
      }
    }

    if (attacker == KING) break;

    scores[index] = see_piece_scores[attacker] - scores[index - 1];
    // printf("%d: player %d, %c attacks %c = %d\n", index, player,
    //        names[attacker], names[victim], scores[index]);
    player = !player;
    victim = attacker;
  }

  // for (int i = 0; i < index; i++) {
  //   printf("%d: %d\n", i, scores[i] * ((i % 2) ? -1 : 1));
  // }

  /* The last piece in the array is the last piece standing, so it is not
     counted for SEE. */
  index--;

  while (--index) {
    // printf("%d: s[n]=%d s[n-1]=%d\n", index, scores[index], scores[index -
    // 1]);
    scores[index - 1] = -max(-scores[index - 1], scores[index]);
    // printf("%d: s[n-1] <- %d\n", index, scores[index - 1]);
  }
  // printf("SEE %d\n", scores[0]);
  return scores[0];
}
