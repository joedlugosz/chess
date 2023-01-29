
#include <stdio.h>
#include <stdlib.h>

#include "hash.h"
#include "io.h"
#include "position.h"

bitboard_t sparse_rand() {
  bitboard_t ret = 0;
  int n_bits = rand() % 8 + 1;
  int position = 0;
  for (int i = 0; i < n_bits; i++) {
    bitboard_t bit;
    do {
      position += rand();
      position %= 64;
      bit = 1ull << position;
    } while (ret & bit);
    ret |= bit;
  }
  return ret;
}

bitboard_t create_rook_blocker_mask(enum square square) {
  int rank = square / 8;
  int file = square % 8;
  bitboard_t relevant = 0ull;
  if (rank < 6)
    for (int i = rank + 1; i < 7; i++) relevant |= 1ull << (i * 8 + file);
  if (rank > 1)
    for (int i = 1; i < rank; i++) relevant |= 1ull << (i * 8 + file);
  if (file < 6)
    for (int i = file + 1; i < 7; i++) relevant |= 1ull << (rank * 8 + i);
  if (file > 1)
    for (int i = 1; i < file; i++) relevant |= 1ull << (rank * 8 + i);
  return relevant;
}

bitboard_t create_rook_moves(enum square square, bitboard_t blockers) {
  int rank = square / 8;
  int file = square % 8;
  bitboard_t moves = 0ull;
  for (int i = rank + 1; i < 8; i++) {
    bitboard_t bit = 1ull << (i * 8 + file);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int i = rank - 1; i >= 0; i--) {
    bitboard_t bit = 1ull << (i * 8 + file);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int i = file + 1; i < 8; i++) {
    bitboard_t bit = 1ull << (rank * 8 + i);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int i = file - 1; i >= 0; i--) {
    bitboard_t bit = 1ull << (rank * 8 + i);
    moves |= bit;
    if (blockers & bit) break;
  }
  return moves;
}

bitboard_t create_bishop_blocker_mask(enum square square) {
  int rank = square / 8;
  int file = square % 8;
  bitboard_t relevant = 0ull;
  if (rank < 6 && file < 6)
    for (int r = rank + 1, f = file + 1; r < 7 && f < 7; r++, f++)
      relevant |= 1ull << (r * 8 + f);
  if (rank > 1 && file > 1)
    for (int r = rank - 1, f = file - 1; r > 0 && f > 0; r--, f--)
      relevant |= 1ull << (r * 8 + f);
  if (rank > 1 && file < 6)
    for (int r = rank - 1, f = file + 1; r > 0 && f < 7; r--, f++)
      relevant |= 1ull << (r * 8 + f);
  if (rank < 6 && file > 1)
    for (int r = rank + 1, f = file - 1; r < 7 && f > 0; r++, f--)
      relevant |= 1ull << (r * 8 + f);
  return relevant;
}

bitboard_t create_bishop_moves(enum square square, bitboard_t blockers) {
  int rank = square / 8;
  int file = square % 8;
  bitboard_t moves = 0ull;
  for (int r = rank + 1, f = file + 1; r < 8 && f < 8; r++, f++) {
    bitboard_t bit = 1ull << (r * 8 + f);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int r = rank - 1, f = file - 1; r > 0 && f > 0; r--, f--) {
    bitboard_t bit = 1ull << (r * 8 + f);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int r = rank - 1, f = file + 1; r > 0 && f < 7; r--, f++) {
    bitboard_t bit = 1ull << (r * 8 + f);
    moves |= bit;
    if (blockers & bit) break;
  }
  for (int r = rank + 1, f = file - 1; r < 7 && f > 0; r++, f--) {
    bitboard_t bit = 1ull << (r * 8 + f);
    moves |= bit;
    if (blockers & bit) break;
  }
  return moves;
}

bitboard_t pack_moves(bitboard_t template, bitboard_t unpacked) {
  bitboard_t packed = 0;
  while (template) {
    packed <<= 1;
    bitboard_t bit = take_next_bit_from(&template);
    if (unpacked & bit) {
      packed |= 1;
    }
  }
  return packed;
}

bitboard_t unpack_moves(bitboard_t template, bitboard_t packed) {
  bitboard_t unpacked = 0;
  while (template) {
    bitboard_t bit = take_next_bit_from(&template);
    if (packed & 1) {
      unpacked |= bit;
    }
    packed >>= 1;
  }
  return unpacked;
}

typedef bitboard_t (*generate_blocker_fn)(enum square);
typedef bitboard_t (*generate_moves_fn)(enum square, bitboard_t);

int find_magics(generate_blocker_fn generate_blocker_mask,
                generate_moves_fn generate_moves, int *shift,
                bitboard_t *magics, bitboard_t *magic_moves) {
  bitboard_t blocker_masks[N_SQUARES];
  bitboard_t blocker_boards[N_SQUARES * 4096];
  bitboard_t generated_moves[N_SQUARES * 4096];

  for (enum square square = A1; square <= H8; square++) {
    bitboard_t blocker_mask = generate_blocker_mask(square);
    blocker_masks[square] = blocker_mask;
    int n_bits = pop_count(blocker_mask);
    for (bitboard_t occupancy = 0; occupancy < (1 << n_bits); occupancy++) {
      bitboard_t blocker_board = unpack_moves(blocker_mask, occupancy);
      blocker_boards[square * 4096 + occupancy] = blocker_board;
      generated_moves[square * 4096 + occupancy] =
          generate_moves(square, blocker_board);
    }
    shift[square] = 64 - n_bits;
  }

  /* Much faster to declare a local array then copy the result */
  bitboard_t moves[4096];

  for (enum square square = A1; square <= H8; square++) {
    bitboard_t template = blocker_masks[square];
    int n_bits = pop_count(template);
    int fail = 0;
    for (int i = 0; i < 1000000; i++) {
      fail = 0;
      bitboard_t magic = sparse_rand();
      memset(moves, -1, sizeof(moves));
      for (bitboard_t test = 1; test < (1 << n_bits) && !fail; test++) {
        bitboard_t blocker_board = blocker_boards[square * 4096 + test];
        bitboard_t hashed = (blocker_board * magic) >> shift[square];
        bitboard_t g_moves = generated_moves[square * 4096 + test];
        if (moves[hashed] == -1ull || moves[hashed] == g_moves) {
          moves[hashed] = g_moves;
        } else {
          fail = 1;
        }
      }
      if (!fail) {
        magics[square] = magic;
        memcpy(&magic_moves[square * 4096], moves, sizeof(moves));
        break;
      }
    }
    if (fail) {
      printf("Can't find magic for square %d\n", square);
      return 1;
    }
  }
  return 0;
}

int check_magics(generate_blocker_fn generate_blocker_mask,
                 generate_moves_fn generate_moves, int *shift,
                 bitboard_t *magics, bitboard_t *magic_moves) {
  for (int i = 0; i < 10000000; i++) {
    int square = rand() & 0x3f;
    bitboard_t blocker_mask = generate_blocker_mask(square);
    bitboard_t occ;
    while (!(
        occ = unpack_moves(
            blocker_mask, rand() & ((1ull << pop_count(blocker_mask)) - 1ull))))
      ;
    bitboard_t g_moves = generate_moves(square, occ);
    bitboard_t hash = (occ * magics[square]) >> shift[square];
    bitboard_t m_moves = magic_moves[square * 4096 + hash];
    if (m_moves != g_moves) {
      printf("Occupancy:\n");
      print_board(0, occ, 0);
      printf("Generated moves:\n");
      print_board(0, g_moves, 0);
      printf("Magic moves:\n");
      print_board(0, m_moves, 0);
      return 1;
    }
  }
  return 0;
}

int main(void) {
  prng_seed(0);
  init_board();

  int rook_magic_shift[N_SQUARES];
  bitboard_t rook_magic_numbers[N_SQUARES];
  bitboard_t *rook_magic_moves =
      malloc(N_SQUARES * 4096 * sizeof(*rook_magic_moves));

  if (find_magics(create_rook_blocker_mask, create_rook_moves, rook_magic_shift,
                  rook_magic_numbers, rook_magic_moves) ||
      check_magics(create_rook_blocker_mask, create_rook_moves,
                   rook_magic_shift, rook_magic_numbers, rook_magic_moves)) {
    free(rook_magic_moves);
    return 1;
  }

  printf("int rook_shift[N_SQUARES] = {\n");
  for (int i = A1; i <= H8; i++) {
    printf("%d,\n", rook_magic_shift[i]);
  }
  printf("};\n\n");
  printf("bitboard_t rook_magic[N_SQUARES] = {\n");
  for (int i = A1; i <= H8; i++) {
    printf("%016llx,\n", rook_magic_numbers[i]);
  }
  printf("};\n");
  free(rook_magic_moves);

  int bishop_magic_shift[N_SQUARES];
  bitboard_t bishop_magic_numbers[N_SQUARES];
  bitboard_t *bishop_magic_moves =
      malloc(N_SQUARES * 4096 * sizeof(*bishop_magic_moves));

  if (find_magics(create_bishop_blocker_mask, create_bishop_moves,
                  bishop_magic_shift, bishop_magic_numbers,
                  bishop_magic_moves) ||
      check_magics(create_bishop_blocker_mask, create_bishop_moves,
                   bishop_magic_shift, bishop_magic_numbers,
                   bishop_magic_moves)) {
    free(bishop_magic_moves);
    return 1;
  }

  printf("int bishop_shift[N_SQUARES] = {\n");
  for (int i = A1; i <= H8; i++) {
    printf("%d,\n", bishop_magic_shift[i]);
  }
  printf("};\n\n");
  printf("bitboard_t bishop_magic[N_SQUARES] = {\n");
  for (int i = A1; i <= H8; i++) {
    printf("%016llx,\n", bishop_magic_numbers[i]);
  }
  printf("};\n");
  free(bishop_magic_moves);

  return 0;
}