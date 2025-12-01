#pragma once

#include <assert.h>

namespace sdk::util {

template<typename T>
struct Maybe {
	bool has;
	T value;

	Maybe() : has(false) { }
	Maybe(T val) : has(true), value(val) { }

	inline T get() const {
		assert(has);
		return value;
	}
	inline void set(T new_val) {
		has = true;
		value = new_val;
	}
	inline void reset() {
		has = false;
	}
};

template<typename T, typename U>
struct Pair {
	T first;
	U second;

	Pair(T first, U second) : first(first), second(second) { }
};

}
