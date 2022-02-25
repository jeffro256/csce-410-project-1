#pragma once

#include <stdint.h>

// modular add that protects against overflow
uint64_t mod_add(uint64_t a, uint64_t b, uint64_t m);

// modular multiply that protects against overflow
uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t m);

// modular exponentiation that protects against overflow
uint64_t mod_pow(uint64_t a, uint64_t e, uint64_t m);

// Miller-Rabin test with a single witness a
// a should !== 0 (mod n)
// m should be odd and (1 << k) * m == n - 1
// n should be an odd integer
// returns 0 if composite, 1 if probably prime (75% chance)
int mr_single_witness(uint64_t a, uint64_t k, uint64_t m, uint64_t n);

/*
Uses a Miller-Rabin test (+ heuristics) to test if n is prime
If deterministic is true, then utilizes the Generalized
Reimann Hypothesis to determine the number of Miller-Rabin
tests required to be deterministic. If deterministic is false,
then does 40 tests. This still has only has a 1 in 4^40 chance
of being incorrect for odd composite numbers, but is around
100x faster than the deterministic mode.
*/
int is_prime(const uint64_t n, int deterministic);