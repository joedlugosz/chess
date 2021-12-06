#ifndef LOWLEVEL_H
#define LOWLEVEL_H

#include "compiler.h"

#if (COMP==GCC)
static inline int ctz(unsigned long long bits) {
  return (int)__builtin_ctzll(bits);
}
static inline int clz(unsigned long long bits) {
  return (int)__builtin_clzll(bits);
}
static inline int pop_count(unsigned long long bits) {
  return (int)__builtin_popcountll(bits);
}
#elif (COMP==MSVC)
#include <intrin.h>
static inline int ctz(unsigned long long bits) {
  unsigned long n = 0;
# ifdef _WIN64
  _BitScanForward64(&n, bits);
  return n;
#else
  if (_BitScanForward(&n, bits & 0xffffffffull)) 
    return n;
  _BitScanForward(&n, bits >> 32);
  return n + 32;
# endif
}
static inline int clz(unsigned long long bits) {
  unsigned long n = 0;
# ifdef _WIN64
  _BitScanReverse64(&n, bits);
  return 63 - n;
#  else
  if (_BitScanReverse(&n, bits >> 32)) {
    return 31 - n;
  }
  _BitScanReverse(&n, bits & 0xffffffffull);
  return 63 - n;
# endif
}
static inline int pop_count(unsigned long long bits) {
  return (int)__popcnt64(bits);
}
#endif

static inline unsigned long long next_bit_from(unsigned long long *bits) {
  unsigned long long ret = *bits & (0ull-*bits);
  *bits = *bits & ~ret;
  return ret;
}
#endif /* LOWLEVEL_H */
