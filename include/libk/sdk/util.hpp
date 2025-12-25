#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <cppsupport.hpp>

namespace sdk::util {

template<typename T>
struct RemoveReference { using type = T; };
template<typename T>
struct RemoveReference<T&> { using type = T; };
template<typename T>
struct RemoveReference<T&&> { using type = T; };

// written using ChatGPT bc I have no idea what I'm doing here
// actually, I have a copy of "The C++ Programming Language",
// I wonder if I could figure this out...
template<typename T>
typename RemoveReference<T>::type&& move(T&& arg) noexcept {
	return static_cast<typename RemoveReference<T>::type&&>(arg);
}

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
		return *(const T*)storage;
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
	using memory_cell_t = uint8_t[sizeof(T)];
	alignas(T*) memory_cell_t *buf;
	size_t len;
	size_t cap;

	void grow_cap() {
		cap *= 2;
		if (cap == 0) cap = 64;
		buf = (memory_cell_t*)realloc(buf, cap);
		assert(buf != NULL);
	}
public:
	using iterator = T*;
	using const_iterator = const T*;

	List(size_t capacity = 64)
	: len(0), cap(capacity) {
		buf = (memory_cell_t*)calloc(capacity, sizeof(T));
		assert(buf != NULL);
		assert(cap != 0);
	}
	~List() {
		if (buf) {
			for (auto &elem : *this) {
				elem.~T();
			}
			free(buf);
		}
	}

	List(const List &other) : List(other.cap) {
		for (const auto &elem : other) {
			this->push_back(elem);
		}
	}
	List &operator=(const List &other) {
		for (auto &elem : *this) {
			elem.~T();
		}
		this->len = 0;
		for (const auto &elem : other) {
			this->push_back(elem);
		}
	}
	List(List &&other) {
		this->buf = other.buf;
		this->len = other.len;
		this->cap = other.cap;
		other.buf = nullptr;
		other.len = 0;
		other.cap = 0;
	}
	List &operator=(List &&other) {
		if (this->buf) {
			for (auto &elem : *this) {
				elem.~T();
			}
			free(this->buf);
		}
		this->buf = other.buf;
		this->len = other.len;
		this->cap = other.cap;
		other.buf = nullptr;
		other.len = 0;
		other.cap = 0;

		return *this;
	}

	const T &operator[](size_t index) const {
		assert(index < len);
		return *(const T*)&buf[index];
	}
	T &operator[](size_t index) {
		assert(index < len);
		return *(T*)&buf[index];
	}

	size_t size() const { return len; }
	size_t capacity() const { return cap; }

	void insert(size_t before, const T &val) {
		assert(before <= len);

		if (before == len) {
			this->push_back(val);
		} else {
			if (len == cap) {
				grow_cap();
			}

			size_t i = len;
			while (i --> before) {
				new ((void*)&buf[i+1]) T(move(*(T*)&buf[i]));
			}

			new ((void*)&buf[before]) T(val);
		}
	}
	void insert(iterator before, const T &val) {
		this->insert(before - begin(), val);
	}
	void insert(size_t before, T &&val) {
		assert(before <= len);

		if (before == len) {
			this->push_back(val);
		} else {
			if (len == cap) {
				grow_cap();
			}

			size_t i = len;
			while (i --> before) {
				new ((void*)&buf[i+1]) T(move(*(T*)&buf[i]));
			}

			new ((void*)&buf[before]) T(val);
		}
	}
	void insert(iterator before, T &&val) {
		this->insert(before - begin(), val);
	}

	void push_back(const T &val) {
		if (len == cap) {
			grow_cap();
		}
		new ((void*)&buf[len++]) T(val);
	}
	void push_back(T &&val) {
		if (len == cap) {
			grow_cap();
		}
		new ((void*)&buf[len++]) T(val);
	}
	T &&pop_back() {
		return move(*(T*)&buf[--len]);
	}

	iterator begin() { return (iterator)&buf[0]; }
	const_iterator begin() const { return (const_iterator)&buf[0]; }
	iterator end() { return (iterator)&buf[len]; }
	const_iterator end() const { return (const_iterator)&buf[len]; }

	void erase(iterator first, iterator last) {
		assert(first <= last);
		assert(begin() <= first);
		assert(last <= end());

		if (first == last) return;

		for (iterator it = first; it != last; ++it) {
			it->~T();
		}

		const size_t erase_len = last - first;

		for (iterator it = last; it != end(); ++it) {
			new ((void*)(it - erase_len)) T(move(*it));
		}

		len -= erase_len;
	}
	void erase(iterator pos) {
		assert(begin() <= pos);
		assert(pos < end());

		erase(pos, pos+1);
	}

	T &back() {
		assert(len > 0);
		return *(T*)&buf[len-1];
	}
	const T &back() const {
		assert(len > 0);
		return *(const T*)&buf[len-1];
	}
};

class String {
	char *buf;
	size_t len;
	size_t cap;

	void grow_cap();
	void add_char_unsafe(char c);
	void add_null_terminator();
public:
	using iterator = char *;
	using const_iterator = const char *;

	String(size_t capacity = 64)
	: len(0), cap(capacity) {
		buf = (char*)calloc(capacity, sizeof(char));
		assert(buf != NULL);
		assert(cap != 0);
	}
	String(const char *str) : String(str, strlen(str)) { }
	String(const char *str, size_t len) : String(len+1) {
		this->len = len;

		// no need for explicit null terminator,
		// buffer is calloc'd so zeroed by default
		for (size_t i = 0; i < len; ++i) {
			buf[i] = str[i];
		}
	}
	~String() {
		if (buf) free(buf);
	}

	String(const String &str) : String(str.buf, str.len) { }
	String &operator=(const String &str) {
		this->len = 0;
		return *this += str;
	}
	String(String &&str) {
		this->buf = str.buf;
		this->len = str.len;
		this->cap = str.cap;
		str.buf = nullptr;
		str.len = 0;
		str.cap = 0;
	}
	String &operator=(String &&str) {
		if (this->buf) free(this->buf);
		this->buf = str.buf;
		this->len = str.len;
		this->cap = str.cap;
		str.buf = nullptr;
		str.len = 0;
		str.cap = 0;

		return *this;
	}

	const char &operator[](size_t index) const;
	char &operator[](size_t index);

	size_t size() const { return len; }
	size_t capacity() const { return cap; }

	// erase at index, min(count, size() - index) characters
	void erase(size_t index = 0, size_t count = ~static_cast<size_t>(0));
	void erase(iterator first, iterator last);
	void erase(iterator position);

	void insert(size_t before, char c);
	void insert(iterator before, char c);
	void insert(size_t before, const char *str);
	void insert(iterator before, const char *str);
	void insert(size_t before, const String &other);
	void insert(iterator before, const String &other);

	// TODO: create string_view type and return substrings as string views
	// (should be much more efficient than creating a new string each time)
	String substr(size_t first, size_t last);
	String substr(iterator first, iterator last);
	String substr(size_t first);
	String substr(iterator first);

	iterator begin() { return &buf[0]; }
	const_iterator begin() const { return &buf[0]; }
	iterator end() { return &buf[len]; }
	const_iterator end() const { return &buf[len]; }

	const char *c_str() const { return buf; }

	String &operator+=(char c);
	String &operator+=(const char *str);
	String &operator+=(const String &other);
	String operator+(char c) const;
	String operator+(const char *str) const;
	String operator+(const String &other) const;

	void write() const;
};

}
