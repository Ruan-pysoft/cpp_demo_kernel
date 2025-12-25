#include <sdk/util.hpp>

#include "vga.hpp"

namespace sdk::util {

void String::grow_cap() {
	cap *= 2;
	if (cap == 0) cap = 64;
	buf = (char*)realloc(buf, cap);
	assert(buf != NULL);
}
void String::add_char_unsafe(char c) {
	if (len == cap) {
		grow_cap();
	}

	buf[len++] = c;
}
void String::add_null_terminator() {
	if (len+1 == cap) {
		grow_cap();
	}

	buf[len] = 0;
}

const char &String::operator[](size_t index) const {
	assert(index < len);
	return buf[index];
}
char &String::operator[](size_t index) {
	assert(index < len);
	return buf[index];
}

void String::erase(size_t index, size_t count) {
	assert(index < len);

	const size_t erase_len = index + count > len ? len - index : count;

	if (erase_len == 0) return;

	for (size_t i = 0; i < erase_len; ++i) {
		buf[index + i] = buf[index + erase_len + i];
	}
	this->len -= erase_len;
	add_null_terminator();
}
void String::erase(iterator first, iterator last) {
	assert(first <= last);
	assert(begin() <= first);
	assert(last <= end());

	erase(first - begin(), last - first);
}
void String::erase(iterator pos) {
	assert(begin() <= pos);
	assert(pos < end());

	erase(pos - begin(), 1);
}

void String::insert(size_t before, char c) {
	assert(before <= len);

	if (before == len) {
		*this += c;
	} else {
		while (len+1 >= cap) {
			grow_cap();
		}

		// include len to also shift up null terminator
		size_t i = len+1;
		while (i --> before) {
			buf[i+1] = buf[i];
		}
		buf[before] = c;
		++len;
	}
}
void String::insert(iterator before, char c) {
	insert(before - begin(), c);
}
void String::insert(size_t before, const char *str) {
	assert(before <= len);

	if (before == len) {
		*this += str;
	} else {
		assert(false && "TODO");
	}
}
void String::insert(iterator before, const char *str) {
	insert(before - begin(), str);
}
void String::insert(size_t before, const String &other) {
	assert(before <= len);

	if (before == len) {
		*this += other;
	} else {
		assert(false && "TODO");
	}
}
void String::insert(iterator before, const String &other) {
	insert(before - begin(), other);
}

String String::substr(size_t first, size_t last) {
	assert(first <= len);
	assert(last <= len);
	assert(first < last);
	return String(&buf[first], last-first);
}
String String::substr(iterator first, iterator last) {
	return substr(first - begin(), last - begin());
}
String String::substr(size_t first) {
	return substr(first, len);
}
String String::substr(iterator first) {
	return substr(first - begin());
}

String &String::operator+=(char c) {
	add_char_unsafe(c);
	add_null_terminator();
	return *this;
}
String &String::operator+=(const char *str) {
	for (size_t i = 0; str[i] != 0; ++i) {
		add_char_unsafe(str[i]);
	}
	add_null_terminator();
	return *this;
}
String &String::operator+=(const String &other) {
	for (const auto ch : other) {
		add_char_unsafe(ch);
	}
	add_null_terminator();
	return *this;
}
String String::operator+(char c) const {
	String res(*this);
	res += c;
	return res;
}
String String::operator+(const char *str) const {
	String res(*this);
	res += str;
	return res;
}
String String::operator+(const String &other) const {
	String res(*this);
	res += other;
	return res;
}

void String::write() const {
	term::write(buf, len);
}

}
