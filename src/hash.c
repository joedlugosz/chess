/*
 *  Transposition table, random number, and Zobrist hashing functions
 */

#include "hash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "position.h"

/*
 * Transposition table size - number of entries, a prime number
 * Zobrist PRNG seed value - hardcoded for repeatable results
 */
enum {
  TT_SIZE = 15485867,
  ZOBRIST_SEED = 4587987,
  N_TRIES = 2,
};

/*
 *  Pseudo-random number generator
 *  TODO: Much better prng scheme
 */

/* Seed the pseudo-random number generator */
void prng_seed(hash_t seed) { srand(seed); }

/* Get a number from the pseudo-random number generator */
hash_t prng_rand(void) { return ((hash_t)rand() << 32) + (hash_t)rand(); }

/*
 *  Zobrist keys
 */
hash_t init_key;
hash_t placement_key[N_PLANES][N_SQUARES];
hash_t castle_rights_key[N_PLAYERS][N_BOARDSIDE];
hash_t turn_key;
hash_t en_passant_key[N_FILES];

/* Initialise the random Zobrist keys */
void hash_init(void) {
  prng_seed(ZOBRIST_SEED);
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

/*
 *  Transposition table
 */

int age;
int updates;
int collisions;

/* Transposition table object */
struct tt_entry *tt;

/* Initialise transposition table memory. Call at program init. */
void tt_init(void) {
  tt = (struct tt_entry *)calloc(TT_SIZE, sizeof(struct tt_entry));
  if (!tt) {
    printf("Can't allocate %lu bytes for transposition table\n",
           TT_SIZE * sizeof(struct tt_entry));
    exit(1);
  }
  age = 0;
}

/* Set a new age - the TT will only probe entries from the current age. */
void tt_new_age(void) {
  age++;
  updates = 0;
  collisions = 0;
}

/* Free transposition table memory. Call at program exit. */
void tt_exit(void) {
  if (tt) free(tt);
}

/* Reset collision counters for transposition table */
void tt_zero(void) {
  updates = 0;
  collisions = 0;
}

/* Return percentage of transposition table updates resulting in collisions
   since last call to tt_zero */
double tt_collisions(void) {
  return updates ? (double)collisions * 100.0 / (double)updates : 0.0;
}

/* Get an entry from the transposition table with the index that corresponds
   to the supplied hash. The entry might not match the hash. */
static inline struct tt_entry *tt_get(hash_t hash) {
  hash_t index = hash % (hash_t)TT_SIZE;
  return &tt[index];
}

/* Update an entry in the transposition table, if the new information is found
   at a greater depth than the existing entry. If the new entry has a hashes a
   different position, a collision is recorded but the update is still made. */
struct tt_entry *tt_update(hash_t hash, enum tt_entry_type type, int depth,
                           score_t score, const struct move *best_move,
                           bitboard_t occupancy) {
  for (hash_t i = 0; i < N_TRIES; i++) {
    struct tt_entry *ret = tt_get(hash + i);

    /* A matching entry has been found but it has been searched to a greater
     * depth - don't update. */
    if (ret->age == age && ret->depth >= depth) return 0;

    /* A hash table collision - an entry has been found from the same age which
     * does not match the hash.  Try the next entry. */
    if (ret->hash != 0 && ret->hash != hash && ret->age == age) continue;

    /* A hash key collision */
    if (ret->hash != 0 && ret->occupancy != occupancy) continue;

    /* Update */
    updates++;
    ret->hash = hash;
    ret->type = type;
    ret->depth = (char)depth;
    ret->score = score;
    ret->age = age;
    ret->occupancy = occupancy;
    if (best_move) memcpy(&ret->best_move, best_move, sizeof(ret->best_move));
    return ret;
  }

  /* After the maximum number of tries, don't update */
  collisions++;
  return 0;
}

/* Probe the transposition table to get an entry which exactly matches the
   supplied hash, or return zero if none is found. */
struct tt_entry *tt_probe(hash_t hash, bitboard_t occupancy) {
  for (hash_t i = 0; i < N_TRIES; i++) {
    struct tt_entry *ret = tt_get(hash + i);
    if (ret->hash != 0 && ret->hash == hash && ret->age == age &&
        ret->occupancy == occupancy) {
      return ret;
    }
  }
  return 0;
}
