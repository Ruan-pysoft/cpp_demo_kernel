#pragma once

#include <assert.h>
#include <stdlib.h>
#include <stdint.h>

#include <cppsupport.hpp>

namespace sdk::util {

template<typename T>
struct Maybe {
	bool has;
	alignas(T) uint8_t storage[sizeof(T)];

	Maybe() : has(false) { }
	Maybe(T val) : has(true) {
		new ((void*)storage) T(val);
	}
	~Maybe() {
		if (has) {
			get().~T();
		}
	}

	inline const T &get() const {
		assert(has);
		return *(T*)storage;
	}
	inline T &get() {
		assert(has);
		return *(T*)storage;
	}
	inline void set(T new_val) {
		reset();
		has = true;
		new (storage) T(new_val);
	}
	inline void reset() {
		if (has) {
			get().~T();
		}
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
