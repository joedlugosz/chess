#ifndef HASH_H
#define HASH_H

#include "evaluate.h"
#include "state.h"

extern hash_t init_key;
extern hash_t placement_key[N_PLANES][N_SQUARES];
extern hash_t castle_rights_key[N_PLAYERS][N_BOARDSIDE];
extern hash_t turn_key;
extern hash_t en_passant_key[N_FILES];

void prng_seed(hash_t seed);
hash_t prng_rand(void);

void hash_init(void);

typedef enum tt_type_e_ { TT_ALPHA, TT_BETA, TT_EXACT } tt_type_e;

typedef struct ttentry_s_ {
  hash_t hash;
  tt_type_e type;
  int depth;
  score_t score;
  move_s best_move;
} ttentry_s;

void tt_init(void);
void tt_clear(void);
ttentry_s *tt_update(hash_t hash, tt_type_e type, int depth, score_t score, move_s *best_move);
ttentry_s *tt_probe(hash_t hash);

#endif /* HASH_H */
