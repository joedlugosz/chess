#include <stdio.h>
#include <stdlib.h>

#include "io.h"
#include "position.h"

const int rook_magic_shift[N_SQUARES] = {
    52, 53, 53, 53, 53, 53, 53, 52, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 53, 54, 54, 54, 54, 54, 54, 53,
    53, 54, 54, 54, 54, 54, 54, 53, 52, 53, 53, 53, 53, 53, 53, 52,
};

const bitboard_t rook_magic_number[N_SQUARES] = {
    0x1080082040008010, 0x0040400020001000, 0x0080098020003004,
    0x0100081001600500, 0x20800a0800040080, 0x0080010400020080,
    0x0280010002002080, 0x0080004100002480, 0xa000800080204010,
    0x8000400250002009, 0x0008802002801000, 0x0001801000180080,
    0x1000808008000400, 0x8012800400820080, 0x0882000401020008,
    0x4100800100284080, 0x0080014000a00040, 0x08c0004020100040,
    0x0001010040200010, 0xa401010010002008, 0x0080808004010800,
    0x0200808004000200, 0x0000840010150802, 0x0000020006840841,
    0x0000400880208001, 0xa400200040005000, 0x0000120600208040,
    0x0010040140080040, 0x0088880080140080, 0x0001000700084400,
    0x4100100400080102, 0x0002408200004401, 0x8001804000800020,
    0x409000a000400840, 0x0810100080802000, 0x1005002009001000,
    0x4a00801800800400, 0x0100804201800400, 0x0000800200800100,
    0x09000100c2000084, 0x1000904000208002, 0x0000400020008080,
    0x0030a00010008080, 0x00021042000a0020, 0x000800800c008008,
    0x0002001008020084, 0x0000210002008080, 0x0001000084410002,
    0x9100800040003080, 0x0000200184c00080, 0x0009009020004300,
    0x0000080280100080, 0x0101080045001100, 0x0084044020100801,
    0x0200020801101c00, 0x0080110041840200, 0x0010102040800301,
    0x2020204000810011, 0x0000200018104501, 0x0000042009001001,
    0x0002001004082102, 0x8002002804100102, 0x0000028910020804,
    0x0000010084007042,
};

const bitboard_t rook_magic_mask[N_SQUARES] = {
    0x000101010101017e, 0x000202020202027c, 0x000404040404047a,
    0x0008080808080876, 0x001010101010106e, 0x002020202020205e,
    0x004040404040403e, 0x008080808080807e, 0x0001010101017e00,
    0x0002020202027c00, 0x0004040404047a00, 0x0008080808087600,
    0x0010101010106e00, 0x0020202020205e00, 0x0040404040403e00,
    0x0080808080807e00, 0x00010101017e0100, 0x00020202027c0200,
    0x00040404047a0400, 0x0008080808760800, 0x00101010106e1000,
    0x00202020205e2000, 0x00404040403e4000, 0x00808080807e8000,
    0x000101017e010100, 0x000202027c020200, 0x000404047a040400,
    0x0008080876080800, 0x001010106e101000, 0x002020205e202000,
    0x004040403e404000, 0x008080807e808000, 0x0001017e01010100,
    0x0002027c02020200, 0x0004047a04040400, 0x0008087608080800,
    0x0010106e10101000, 0x0020205e20202000, 0x0040403e40404000,
    0x0080807e80808000, 0x00017e0101010100, 0x00027c0202020200,
    0x00047a0404040400, 0x0008760808080800, 0x00106e1010101000,
    0x00205e2020202000, 0x00403e4040404000, 0x00807e8080808000,
    0x007e010101010100, 0x007c020202020200, 0x007a040404040400,
    0x0076080808080800, 0x006e101010101000, 0x005e202020202000,
    0x003e404040404000, 0x007e808080808000, 0x7e01010101010100,
    0x7c02020202020200, 0x7a04040404040400, 0x7608080808080800,
    0x6e10101010101000, 0x5e20202020202000, 0x3e40404040404000,
    0x7e80808080808000,
};

const int bishop_magic_shift[N_SQUARES] = {
    58, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 59, 59,
    59, 59, 57, 57, 57, 57, 59, 59, 59, 59, 57, 55, 55, 57, 59, 59,
    59, 59, 57, 55, 55, 57, 59, 59, 59, 59, 57, 57, 57, 57, 59, 59,
    59, 59, 59, 59, 59, 59, 59, 59, 58, 59, 59, 59, 59, 59, 59, 58,
};

const bitboard_t bishop_magic_number[N_SQUARES] = {
    0x0002080104018200, 0x0088020800410010, 0x00020400d0810200,
    0x00c2008110410000, 0x00040d0000000000, 0x0012080200021000,
    0x000212c120002000, 0x0000490808020040, 0x0000090830008200,
    0x2005100401040020, 0x8000100400882c00, 0x0000120212002001,
    0x0210022201000000, 0x4800020a82001000, 0x0800088404020000,
    0x0000020042024014, 0x0800306008010100, 0x0000041806020140,
    0x4000000088428104, 0x800002080011000a, 0x0000000202100000,
    0x1040000230050010, 0x0200000406010000, 0x0040000602020042,
    0x0140100108820800, 0x0400020088020500, 0x0000040000482200,
    0x0000002011840004, 0x1000004800208008, 0x0042040400808000,
    0x0080008001040000, 0x0000004000820040, 0x0000044002201224,
    0x0080020840300100, 0x1400010400110040, 0x0000000400018020,
    0x9100810200010008, 0x0000020400009000, 0x4800088108020000,
    0x000a021200202000, 0x8000081290204000, 0x0040009028019000,
    0x2000004100800040, 0x0000000064808200, 0x0000a80401000020,
    0x0100020141400010, 0x0140020202000000, 0x0080010200204012,
    0x0200208220240000, 0x00008c0101101000, 0x0000000108810804,
    0x100000040a020004, 0x0010600010040020, 0x1200000808980000,
    0x1048200102021000, 0x00800200a1010000, 0x008004008c040220,
    0x00200000c1041014, 0x0020400000414450, 0x9000001000024801,
    0x4040400000a40400, 0x0000100210100080, 0x0000810841048080,
    0x8000040800404200,
};

const bitboard_t bishop_magic_mask[N_SQUARES] = {
    0x0040201008040200, 0x0000402010080400, 0x0000004020100a00,
    0x0000000040221400, 0x0000000002442800, 0x0000000204085000,
    0x0000020408102000, 0x0002040810204000, 0x0020100804020000,
    0x0040201008040000, 0x00004020100a0000, 0x0000004022140000,
    0x0000000244280000, 0x0000020408500000, 0x0002040810200000,
    0x0004081020400000, 0x0010080402000200, 0x0020100804000400,
    0x004020100a000a00, 0x0000402214001400, 0x0000024428002800,
    0x0002040850005000, 0x0004081020002000, 0x0008102040004000,
    0x0008040200020400, 0x0010080400040800, 0x0020100a000a1000,
    0x0040221400142200, 0x0002442800284400, 0x0004085000500800,
    0x0008102000201000, 0x0010204000402000, 0x0004020002040800,
    0x0008040004081000, 0x00100a000a102000, 0x0022140014224000,
    0x0044280028440200, 0x0008500050080400, 0x0010200020100800,
    0x0020400040201000, 0x0002000204081000, 0x0004000408102000,
    0x000a000a10204000, 0x0014001422400000, 0x0028002844020000,
    0x0050005008040200, 0x0020002010080400, 0x0040004020100800,
    0x0000020408102000, 0x0000040810204000, 0x00000a1020400000,
    0x0000142240000000, 0x0000284402000000, 0x0000500804020000,
    0x0000201008040200, 0x0000402010080400, 0x0002040810204000,
    0x0004081020400000, 0x000a102040000000, 0x0014224000000000,
    0x0028440200000000, 0x0050080402000000, 0x0020100804020000,
    0x0040201008040200,
};

typedef bitboard_t (*generate_moves_fn)(enum square, bitboard_t);

bitboard_t generate_rook_moves(enum square square, bitboard_t blockers) {
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

bitboard_t generate_bishop_moves(enum square square, bitboard_t blockers) {
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

void init_magic_moves(generate_moves_fn generate_moves, const int *shift,
                      const bitboard_t *magic_mask,
                      const bitboard_t *magic_number, bitboard_t *magic_moves) {
  for (enum square square = A1; square <= H8; square++) {
    bitboard_t blocker_mask = magic_mask[square];
    int n_bits = pop_count(blocker_mask);
    for (bitboard_t occupancy = 0; occupancy < (1 << n_bits); occupancy++) {
      bitboard_t blocker_board = unpack_moves(blocker_mask, occupancy);
      bitboard_t hash = (blocker_board * magic_number[square]) >> shift[square];
      bitboard_t *entry = &magic_moves[square * 4096 + hash];
      if (!*entry) *entry = generate_moves(square, blocker_board);
    }
  }
}

int check_magics(generate_moves_fn generate_moves, const int *shift,
                 const bitboard_t *blocker_mask, const bitboard_t *magics,
                 const bitboard_t *magic_moves) {
  for (int square = 0; square < 64; square++) {
    for (bitboard_t test = 1; test < 4096; test++) {
      bitboard_t occ = unpack_moves(blocker_mask[square], test);
      bitboard_t g_moves = generate_moves(square, occ);
      bitboard_t hash = (occ * magics[square]) >> shift[square];
      bitboard_t m_moves = magic_moves[square * 4096 + hash];
      if (m_moves != g_moves) {
        printf("Occupancy:\n");
        print_board(0, occ, 1ull << square);
        printf("Generated moves:\n");
        print_board(0, g_moves, 0);
        printf("Magic moves:\n");
        print_board(0, m_moves, 0);
      }
    }
  }
  return 0;
}

int main(void) {
  init_board();
  bitboard_t *rook_magic_moves =
      malloc(N_SQUARES * 4096 * sizeof(*rook_magic_moves));
  init_magic_moves(generate_rook_moves, rook_magic_shift, rook_magic_mask,
                   rook_magic_number, rook_magic_moves);
  check_magics(generate_rook_moves, rook_magic_shift, rook_magic_mask,
               rook_magic_number, rook_magic_moves);
  bitboard_t *bishop_magic_moves =
      malloc(N_SQUARES * 4096 * sizeof(*bishop_magic_moves));
  init_magic_moves(generate_bishop_moves, bishop_magic_shift, bishop_magic_mask,
                   bishop_magic_number, bishop_magic_moves);
  check_magics(generate_bishop_moves, bishop_magic_shift, bishop_magic_mask,
               bishop_magic_number, bishop_magic_moves);
}