
#include "chess.h"
#include "hash.h"
#include <malloc.h>
#include <stdlib.h>

hash_t *table;

void init_hash(hash_s *h, unsigned int n_values, unsigned int size)
{
  h->n_values = n_values;
  h->values = (hash_t *)malloc(n_values * sizeof(hash_t));
  h->table = (hash_t *)calloc(size * sizeof(hash_t), 0);
  
  for(int i = 0; i < n_values; i++) {
    h->values[i] = rand();
  }
}

void write_hash(hash_t *table, hash_t value)
{
  hash_t index = value >> 48;
  table[index] = value;
}

hash_t read_hash(hash_t *table)
{
  hash_t index = value >> 48;
  return table[index];
}
