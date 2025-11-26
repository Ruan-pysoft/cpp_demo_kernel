#include "apps/forth.hpp"

#include <assert.h>
#include <cstdint>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace forth {

namespace {

constexpr size_t MAX_LINE_LEN = vga::WIDTH - 3;
constexpr size_t STACK_SIZE = 1024;
struct State {
	bool should_quit = false;
	bool capslock = false;
	size_t line_len = 0;
	char line[MAX_LINE_LEN];
	size_t stack_len = 0;
	uint32_t stack[STACK_SIZE];
	bool has_inp_err = false;
	uint32_t inp_err_until = 0;

	State(const State&) = delete;
	State &operator=(const State&) = delete;
	State(State&&) = default;
	State &operator=(State&&) = default;
};
static State state{};
bool forth_running = false;

static inline void stack_push(uint32_t val) {
	// WARN: I assume here that there is space on the stack, without verifying the fact!
	state.stack[state.stack_len++] = val;
}
static inline uint32_t stack_pop() {
	// WARN: I assume here that the stack is non-empty, without verifying the fact!
	return state.stack[--state.stack_len];
}
static inline uint32_t stack_peek(size_t nth = 1) {
	// WARN: I assume here that the stack's length is >= nth, without verifying the fact!
	return state.stack[state.stack_len - nth];
}
static inline uint32_t &stack_get(size_t nth = 1) {
	// WARN: I assume here that the stack's length is >= nth, without verifying the fact!
	return state.stack[state.stack_len - nth];
}
struct PrimitiveEntry {
	const char *name;
	void(*func)();
};
const PrimitiveEntry primitives[] = {
	{ "dup", []() {
		assert(state.stack_len >= 1);
		assert(state.stack_len < STACK_SIZE);
		stack_push(stack_peek());
	} },
	{ "inc", []() {
		assert(state.stack_len >= 1);
		++stack_get();
	} },
	{ "dec", []() {
		assert(state.stack_len >= 1);
		--stack_get();
	} },
	{ "+", []() {
		assert(state.stack_len >= 2);
		stack_push(stack_pop() + stack_pop());
	} },
	{ "print", []() {
		assert(state.stack_len >= 1);
		printf("%d ", reinterpret_cast<uint32_t>(stack_pop()));
	} },
	{ "pstr", []() {
		assert(state.stack_len >= 1);
		const uint32_t str_raw = stack_pop();
		const char *str = (char*)&str_raw;
		for (size_t i = 0; i < 4; ++i) {
			if (str[i] == 0) break;
			putchar(str[i]);
		}
	} },
	{ "stack_len", []() {
		assert(state.stack_len < STACK_SIZE);
		stack_push(state.stack_len);
	} },
	{ "exit", []() {
		state.should_quit = true;
	} },
	{ "quit", []() {
		state.should_quit = true;
	} },
};
constexpr size_t primitives_len = sizeof(primitives)/sizeof(*primitives);

void input_key(ps2::Key, char ch, bool capitalise) {
	if (state.line_len == MAX_LINE_LEN) {
		state.has_inp_err = true;
		state.inp_err_until = pit::millis + 100;

		term::setcolor(vga::entry_color(
			vga::Color::Black,
			vga::Color::LightRed
		));

		term::advance();
		term::backspace();

		term::resetcolor();
		return;
	}
	// TODO: capitalisation for accented letters?
	// wait, I can't enter those on the keyboard anyways...
	// first I'd need a way to enter them
	if (capitalise && 'a' <= ch && ch <= 'z') ch ^= 'a'^'A';
	state.line[state.line_len++] = ch;
	putchar(ch);

	if (state.line_len == MAX_LINE_LEN) term::cursor::disable();
}

void interpret_line() {
	size_t word_start = 0;
	size_t word_end = 0;

	while (word_start < state.line_len) {
		while (word_start < state.line_len && state.line[word_start] == ' ') {
			++word_start;
		}
		word_end = word_start;
		while (word_end < state.line_len && state.line[word_end] != ' ') {
			++word_end;
		}

		if (word_start == word_end) break;

		bool was_primitive = false;
		for (size_t pi = 0; pi < primitives_len; ++pi) {
			if (strlen(primitives[pi].name) != word_end-word_start) continue;

			bool matches = true;
			for (size_t i = 0; i < word_end-word_start; ++i) {
				if (primitives[pi].name[i] != state.line[word_start+i]) {
					matches = false;
					break;
				}
			}

			if (matches) {
				was_primitive = true;
				primitives[pi].func();

				break;
			}
		}

		if (!was_primitive) {
			bool is_number = true;
			uint32_t number = 0;
			for (size_t i = 0; i < word_end-word_start; ++i) {
				const char ch = state.line[word_start+i];
				if (ch < '0' || ch > '9') {
					is_number = false;
					break;
				} else {
					number *= 10;
					number += ch - '0';
				}
			}

			if (is_number) {
				assert(state.stack_len < STACK_SIZE);
				state.stack[state.stack_len++] = number;
			} else {
				assert(false && "Err: Undefined word");
			}
		}

		word_start = word_end;
	}

	putchar('\n');
}

void handle_keyevent(ps2::EventType type, ps2::Key key) {
	using namespace ps2;

	if (type != EventType::Press && type != EventType::Bounce) return;

	if (key_ascii_map[key] && key != KEY_ENTER) {
		const bool capitalise = key_state[KEY_LSHIFT]
			|| key_state[KEY_RSHIFT]
			|| state.capslock;
		input_key(key, key_ascii_map[key], capitalise);
		return;
	}

	if (key == KEY_BACKSPACE) {
		if (state.line_len == 0) return;

		if (state.has_inp_err) {
			state.has_inp_err = false;

			term::advance();
			term::backspace();
		}

		--state.line_len;
		term::backspace();
		term::cursor::enable(8, 15);
	}

	if (key == KEY_ENTER) {
		if (state.has_inp_err) {
			state.has_inp_err = false;

			term::advance();
			term::backspace();
		}

		putchar('\n');
		interpret_line();
		state.line_len = 0;
		term::writestring("> ");
		term::cursor::enable(8, 15);
		return;
	}
}

}

void main() {
	assert(!forth_running);
	forth_running = true;
	state = State{};

	term::clear();
	term::go_to(0, 0);
	term::writestring("> ");

	while (!state.should_quit) {
		while (!ps2::events.empty()) {
			const auto event = ps2::events.pop();
			handle_keyevent(event.type, event.key);
		}

		if (state.has_inp_err && pit::millis >= state.inp_err_until) {
			state.has_inp_err = false;

			term::advance();
			term::backspace();
		}

		__asm__ volatile("hlt" ::: "memory");
	}

	forth_running = false;
}

}
