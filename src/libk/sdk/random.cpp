#include <sdk/random.hpp>

#include <stdint.h>

#include "pit.hpp"

namespace sdk::random {

namespace {

static Xorshift32 sys_prng {};

}

Xorshift32::Xorshift32() : Xorshift32(pit::millis) { }
// XOR'ing seed with 0b10101010.... to get a good distribution of active bits initially
Xorshift32::Xorshift32(uint32_t seed) : state(seed ^ 0xAA'AA'AA'AA) {
	if (seed == 0) {
		// seed shouldn't be zero!
		seed = pit::millis; // assume that pit::millis is never zero
	}
}

uint32_t Xorshift32::next() {
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return state;
}

void seed(uint32_t seed) {
	sys_prng = Xorshift32(seed);
}
uint32_t random() {
	const uint32_t res = sys_prng.next();

	if (res == 0) {
		sys_prng = Xorshift32(pit::millis);

		return sys_prng.next();
	}

	return res;
}

}
