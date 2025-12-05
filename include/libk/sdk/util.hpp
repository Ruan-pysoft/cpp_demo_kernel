#pragma once

#include <assert.h>
#include <stdlib.h>

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

template<typename T>
class List {
	T *buf;
	size_t len;
	size_t cap;
public:
	using iterator = T*;

	List(size_t capacity = 64)
	: buf((T*)calloc(capacity, sizeof(T))), len(0), cap(capacity) {
		assert(buf != NULL);
		assert(cap != 0);
	}

	const T &operator[](size_t index) const {
		assert(index < len);
		return buf[index];
	}
	T &operator[](size_t index) {
		assert(index < len);
		return buf[index];
	}

	size_t size() const { return len; }
	size_t capacity() const { return cap; }

	void push_back(const T &val) {
		if (len == cap) {
			cap *= 2;
			buf = (T*)reallocarray(buf, cap, sizeof(T));
			assert(buf != NULL);
		}
		buf[len++] = val;
	}
	void push_back(T &&val) {
		if (len == cap) {
			cap *= 2;
			buf = (T*)reallocarray(buf, cap, sizeof(T));
			assert(buf != NULL);
		}
		buf[len++] = val;
	}
	T pop_back() {
		return buf[--len];
	}

	T *begin() {
		return &buf[0];
	}
	T *end() {
		return &buf[len];
	}
};

}
