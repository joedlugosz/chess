
#include <stdio.h>
#include <stdlib.h>

#include "hash.h"
#include "io.h"
#include "position.h"

int shift[N_SQUARES];
bitboard_t rook_magic[N_SQUARES];
bitboard_t rook_magic_moves[64 * 4096];

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

int main(void) {
  prng_seed(0);
  init_board();

  bitboard_t rook_blocker_masks[N_SQUARES];
  bitboard_t rook_blocker_boards[N_SQUARES * 4096];
  bitboard_t rook_moves[N_SQUARES * 4096];

  printf("int rook_shift[N_SQUARES] = {\n");

  for (enum square square = A1; square <= H8; square++) {
    bitboard_t blocker_mask = create_rook_blocker_mask(square);
    rook_blocker_masks[square] = blocker_mask;
    int n_bits = pop_count(blocker_mask);
    for (bitboard_t occupancy = 0; occupancy < (1 << n_bits); occupancy++) {
      bitboard_t blocker_board = unpack_moves(blocker_mask, occupancy);
      rook_blocker_boards[square * 4096 + occupancy] = blocker_board;
      rook_moves[square * 4096 + occupancy] =
          create_rook_moves(square, blocker_board);
    }
    shift[square] = 64 - n_bits;
    printf("%d,\n", shift[square]);
  }
  printf("};\n\n");

  printf("bitboard_t rook_magic[N_SQUARES] = {\n");

  bitboard_t moves[4096];
  for (enum square square = A1; square <= H8; square++) {
    bitboard_t template = rook_blocker_masks[square];
    int n_bits = pop_count(template);
    int fail = 0;
    for (int i = 0; i < 1000000; i++) {
      fail = 0;
      bitboard_t magic = sparse_rand();
      memset(moves, -1, sizeof(moves));
      for (bitboard_t test = 1; test < (1 << n_bits) && !fail; test++) {
        bitboard_t blocker_board = rook_blocker_boards[square * 4096 + test];
        bitboard_t hashed = (blocker_board * magic) >> shift[square];
        bitboard_t mmoves = rook_moves[square * 4096 + test];
        if (moves[hashed] == -1ull || moves[hashed] == mmoves) {
          moves[hashed] = mmoves;
        } else {
          fail = 1;
        }
      }
      if (!fail) {
        printf("  0x%016llx,\n", magic);
        rook_magic[square] = magic;
        memcpy(&rook_magic_moves[square * 4096], moves, sizeof(moves));
        break;
      }
    }
    if (fail) {
      printf("Can't find magic for square %d\n", square);
      return 1;
    }
  }
  printf("};\n");

  for (int i = 0; i < 10000000; i++) {
    int square = rand() & 0x3f;
    bitboard_t blocker_mask = create_rook_blocker_mask(square);
    bitboard_t occ;
    while (!(
        occ = unpack_moves(
            blocker_mask, rand() & ((1ull << pop_count(blocker_mask)) - 1ull))))
      ;
    bitboard_t generated_moves = create_rook_moves(square, occ);
    bitboard_t hash = (occ * rook_magic[square]) >> shift[square];
    bitboard_t magic_moves = rook_magic_moves[square * 4096 + hash];
    if (magic_moves != generated_moves) {
      printf("Occupancy:\n");
      print_board(0, occ, 0);
      printf("Generated moves:\n");
      print_board(0, generated_moves, 0);
      printf("Magic moves:\n");
      print_board(0, magic_moves, 0);
    }
  }

  return 0;
}