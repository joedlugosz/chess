/*
 *   Low-level bit functions
 */

#ifndef LOWLEVEL_H
#define LOWLEVEL_H

#include "compiler.h"

/* Remove the least significant bit from `bits` and return a word with
 * that bit set, or zero if `bits` is zero.
 * Can be used in a while loop to iterate over all set bits in a word */
static inline unsigned long long take_next_bit_from(unsigned long long *bits) {
  unsigned long long ret = *bits & (0ull - *bits);
  *bits = *bits & ~ret;
  return ret;
}

/* Compiler intrinsics */

#if defined(__clang__) || defined(__GNUC__)

/* Count trailing zero bits in word */
static inline int ctz(unsigned long long bits) { return (int)__builtin_ctzll(bits); }

/* Count leading zero bits in word */
static inline int clz(unsigned long long bits) { return (int)__builtin_clzll(bits); }

/* Count set bits in word */
static inline int pop_count(unsigned long long bits) { return (int)__builtin_popcountll(bits); }

#elif defined(_MSC_VER)
#  include <intrin.h>

/* Count trailing zero bits in word */
static inline int ctz(unsigned long long bits) {
  unsigned long n = 0;
#  if defined(_WIN64)
  _BitScanForward64(&n, bits);
  return n;
#  else
  if (_BitScanForward(&n, bits & 0xffffffffull)) return n;
  _BitScanForward(&n, bits >> 32);
  return n + 32;
#  endif
}

/* Count leading zero bits in word */
static inline int clz(unsigned long long bits) {
  unsigned long n = 0;
#  if defined(_WIN64)
  _BitScanReverse64(&n, bits);
  return 63 - n;
#  else
  if (_BitScanReverse(&n, bits >> 32)) {
    return 31 - n;
  }
  _BitScanReverse(&n, bits & 0xffffffffull);
  return 63 - n;
#  endif
}

/* Count set bits in word */
static inline int pop_count(unsigned long long bits) { return (int)__popcnt64(bits); }
#endif /* defined (__clang__ _GNUC_ _MSC_VER) */

#endif /* LOWLEVEL_H */
