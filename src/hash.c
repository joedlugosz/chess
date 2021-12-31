#include "hash.h"

#include <stdlib.h>

#include "state.h"

enum { N_HASH = N_PLANES * N_SQUARES };

hash_t init_key;
hash_t placement_key[N_PLANES][N_SQUARES];
hash_t castle_rights_key[N_PLAYERS][N_BOARDSIDE];
hash_t turn_key;
hash_t en_passant_key[N_FILES];

void prng_seed(hash_t seed) { srand(seed); }

hash_t prng_rand(void) { return rand(); }

void hash_init(void) {
  prng_seed(123);
  init_key = prng_rand();
  turn_key = prng_rand();
  for (int i = 0; i < N_PLANES; i++) {
    for (int j = 0; j < N_SQUARES; j++) {
      placement_key[i][j] = prng_rand();
    }
  }
  for (int i = 0; i < N_PLAYERS; i++) {
    for (int j = 0; j < N_BOARDSIDE; j++) {
      castle_rights_key[i][j] = prng_rand();
    }
  }
  for (int i = 0; i < N_FILES; i++) {
    en_passant_key[i] = prng_rand();
  }
}

enum { TT_SIZE = 1 << 24 };
ttentry_s tt[TT_SIZE];

void tt_clear(void) { memset(tt, 0, sizeof(tt)); }

ttentry_s *tt_get(hash_t hash) {
  hash_t index = hash % (hash_t)TT_SIZE;
  return &tt[index];
}

ttentry_s *tt_update(hash_t hash, int depth, score_t score, move_s *best_move) {
  ttentry_s *ret = tt_get(hash);
  ret->hash = hash;
  ret->depth = depth;
  ret->score = score;
  memcpy(&ret->best_move, best_move, sizeof(ret->best_move));
  return ret;
}

ttentry_s *tt_grab(hash_t hash) {
  ttentry_s *ret = tt_get(hash);
  memset(ret, 0, sizeof(*ret));
  ret->hash = hash;
  return ret;
}

ttentry_s *tt_probe(hash_t hash) {
  ttentry_s *ret = tt_get(hash);
  if (ret->hash != hash) {
    printf("TT coll %llx %llx\n", ret->hash, hash);
    return 0;
  }
  return ret;
}
