#pragma once

#include <stdint.h>

namespace sdk::random {

template<typename T>
class Prng {
public:
	virtual T next() = 0;
};

class Xorshift32 : public Prng<uint32_t> {
	// see https://en.wikipedia.org/wiki/Xorshift
	uint32_t state;
public:
	Xorshift32();
	Xorshift32(uint32_t seed);

	uint32_t next() override;
};

void seed(uint32_t seed);
uint32_t random();

}
