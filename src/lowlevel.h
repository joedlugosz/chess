#ifndef LOWLEVEL_H
#define LOWLEVEL_H

#include "compiler.h"

#if (COMP==GCC)
static inline int ctz(plane_t mask) {
  return (int)__builtin_ctzll(mask);
}
static inline int clz(plane_t mask) {
  return (int)__builtin_clzll(mask);
}
static inline int pop_count(plane_t mask) {
  return (int)__builtin_popcountll(mask);
}
#elif (COMP==MSVC)
#include <intrin.h>
static inline int ctz(plane_t mask) {
  unsigned long n = 0;
# ifdef _WIN64
  _BitScanForward64(&n, mask);
  return n;
#else
  if (_BitScanForward(&n, mask & 0xffffffffull)) 
    return n;
  _BitScanForward(&n, mask >> 32);
  return n + 32;
# endif
}
static inline int clz(plane_t mask) {
  unsigned long n = 0;
# ifdef _WIN64
  _BitScanReverse64(&n, mask);
  return 63 - n;
#  else
  if (_BitScanReverse(&n, mask >> 32)) {
    return 31 - n;
  }
  _BitScanReverse(&n, mask & 0xffffffffull);
  return 63 - n;
# endif
}
static inline int pop_count(plane_t mask) {
  return (int)__popcnt64(mask);
}
#endif

static inline plane_t next_bit_from(plane_t *bits) {
  plane_t ret = *bits & (0ull-*bits);
  *bits = *bits & ~ret;
  return ret;
}
#endif /* LOWLEVEL_H */
