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

void String::erase(size_t pos, size_t len) {
	assert(pos <= this->len && pos + len <= this->len);

	if (len == 0) return;

	for (size_t i = 0; pos + len + i < this->len; ++i) {
		buf[pos + i] = buf[pos + len + i];
	}
	this->len -= len;
	add_null_terminator();
}
void String::erase(size_t from) {
	this->erase(from, len - from);
}
void String::erase(String::const_iterator a, String::const_iterator b) {
	assert(a <= b);
	assert(this->begin() <= a);
	assert(b <= this->end());

	this->erase(a - this->begin(), b - a);
}
void String::erase(String::const_iterator a) {
	this->erase(a, this->end());
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

void String::write() const {
	term::write(buf, len);
}

}
