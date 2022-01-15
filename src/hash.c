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

/* TODO: better prng scheme */
hash_t prng_rand(void) { return ((hash_t)rand() << 32) + (hash_t)rand(); }

void hash_init(void) {
  prng_seed(4587987);
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

/* Transposition table size - prime number */
enum { TT_SIZE = 15485867 };

ttentry_s *tt;

void tt_init(void) {
  tt = (ttentry_s *)calloc(TT_SIZE, sizeof(ttentry_s));
  if (!tt) {
    printf("Can't allocate %lu bytes for transposition table\n", TT_SIZE * sizeof(ttentry_s));
    exit(1);
  }
}

static inline ttentry_s *tt_get(hash_t hash) {
  hash_t index = hash % (hash_t)TT_SIZE;
  return &tt[index];
}

ttentry_s *tt_update(hash_t hash, tt_type_e type, int depth, score_t score, move_s *best_move) {
  ttentry_s *ret = tt_get(hash);
  ret->hash = hash;
  ret->type = type;
  ret->depth = depth;
  ret->score = score;
  memcpy(&ret->best_move, best_move, sizeof(ret->best_move));
  return ret;
}

ttentry_s *tt_probe(hash_t hash) {
  ttentry_s *ret = tt_get(hash);
  if (ret->hash != hash) {
    return 0;
  }
  return ret;
}
