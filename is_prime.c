#include <stdint.h>

#include "is_prime.h"

uint64_t mod_add(uint64_t a, uint64_t b, uint64_t m) {
	a = a % m;
	b = b % m;

	if (m - a > b) {
		return a + b;
	} else if (m - a < b) {
		return b - (m - a);
	} else {
		return 0;
	}
}

uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t m) {
	uint64_t res = 0;

	while (b > 0) {
		if (b & 1) {
			res = mod_add(res, a, m);
		}

		a = mod_add(a, a, m);
		b >>= 1;
	}

	return res;
}

uint64_t mod_pow(uint64_t a, uint64_t e, uint64_t m) {
	uint64_t res = 1;

	while (e > 0) {
		if (e & 1) {
			res = mod_mul(res, a, m);
		}

		a = mod_mul(a, a, m);
		e >>= 1;
	}

	return res;
}

int mr_single_witness(uint64_t a, uint64_t k, uint64_t m, uint64_t n) {
	uint64_t b = mod_pow(a, m, n);

	if (b == 1) {
		return 1;
	}

	for (int i = 0; i < k; i++) {
		if (b == n - 1) {
			return 1;
		} else if (b == 1) {
			return 0;
		}

		b = mod_mul(b, b, n);
	}

	return 0;
}

int is_prime(const uint64_t n, int deterministic) {
	// The largest theoretical first miller-rabin witness for any n < 2^64 
	// = ceil(2 * ln(2^64)^2)
	const uint64_t reimann_limit_64 = 3936;
	// faster, but non-deterministic
	const uint64_t nondeterministic_limit = 40;
	const uint64_t algo_limit = deterministic ?
		reimann_limit_64 : nondeterministic_limit;

	if (n < 2) { // == 0 or == 1
		return 0;
	} else if (n < 4) { // == 2 or == 3
		return 1;
	} else if (n % 2 == 0) { // if even
		return 0;
	}

	// we now know n >= 5 and odd
	// find k and m such that: 2^k * m == n - 1 and m is odd
	uint64_t k = 1;
	uint64_t m = (n - 1) >> 1;
	while (m & 1 == 0) {
		k++;
		m >>= 1;
	}

	// Do deterministic miller-rabin test
	// Test witnesses in range [2, reimann_limit_64] unless n is small
	const uint64_t max_witness = (n <= algo_limit) ? (n - 1) : algo_limit;
	for (uint64_t a = 2; a <= max_witness; a++) {
		if (!mr_single_witness(a, k, m, n)) {
			return 0;
		}
	}
	return 1;
}