/*
 *  Transposition table, random number, and Zobrist hashing functions
 */

#ifndef HASH_H
#define HASH_H

#include "evaluate.h"
#include "position.h"

/*
 *  Zobrist keys
 */
extern hash_t init_key;
extern hash_t placement_key[N_PLANES][N_SQUARES];
extern hash_t castle_rights_key[N_PLAYERS][N_BOARDSIDE];
extern hash_t turn_key;
extern hash_t en_passant_key[N_FILES];
void hash_init(void);

/*
 *  Pseudo-random number generator
 *  TODO: Much better prng scheme
 */
void prng_seed(hash_t seed);
hash_t prng_rand(void);

/*
 *  Transposition table
 */

/* Transposition table entry type */
typedef enum tt_type_e_ {
  TT_ALPHA,
  TT_BETA,
  TT_EXACT,
} tt_type_e;

/* Transposition table entry */
typedef struct ttentry_s_ {
  hash_t hash;
  tt_type_e type;
  int depth;
  score_t score;
  struct move best_move;
} ttentry_s;

void tt_exit(void);
void tt_zero(void);
double tt_collisions(void);
void tt_init(void);
void tt_clear(void);
ttentry_s *tt_update(hash_t hash, tt_type_e type, int depth, score_t score, struct move *best_move);
ttentry_s *tt_probe(hash_t hash);

#endif /* HASH_H */
