#include "apps/forth.hpp"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace forth {

namespace {

struct InterpreterState {
	size_t line_pos = 0;
	size_t curr_word_pos = 0;
	size_t curr_word_len = 0;
};

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
	InterpreterState interp;

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
void get_word();
const PrimitiveEntry primitives[] = {
	{ "dup", []() {
		assert(state.stack_len >= 1);
		assert(state.stack_len < STACK_SIZE);
		stack_push(stack_peek());
	} },
	{ "swap", []() {
		assert(state.stack_len >= 2);
		assert(state.stack_len < STACK_SIZE);
		const uint32_t top = stack_pop();
		const uint32_t under_top = stack_pop();
		stack_push(top);
		stack_push(under_top);
	} },
	{ "rot", []() {
		// ( a b c -- b c a )
		assert(state.stack_len >= 2);
		assert(state.stack_len < STACK_SIZE);
		const uint32_t c = stack_pop();
		const uint32_t b = stack_pop();
		const uint32_t a = stack_pop();
		stack_push(b);
		stack_push(c);
		stack_push(a);
	} },
	{ "drop", []() {
		assert(state.stack_len >= 1);
		stack_pop();
	} },
	{ "inc", []() {
		assert(state.stack_len >= 1);
		++stack_get();
	} },
	{ "dec", []() {
		assert(state.stack_len >= 1);
		--stack_get();
	} },
	{ "shl", []() {
		assert(state.stack_len >= 2);
		const uint32_t top = stack_pop();
		const uint32_t under_top = stack_pop();
		stack_push(under_top << top);
	} },
	{ "shr", []() {
		assert(state.stack_len >= 2);
		const uint32_t top = stack_pop();
		const uint32_t under_top = stack_pop();
		stack_push(under_top >> top);
	} },
	{ "or", []() {
		assert(state.stack_len >= 2);
		stack_push(stack_pop() | stack_pop());
	} },
	{ "and", []() {
		assert(state.stack_len >= 2);
		stack_push(stack_pop() & stack_pop());
	} },
	{ "xor", []() {
		assert(state.stack_len >= 2);
		stack_push(stack_pop() ^ stack_pop());
	} },
	{ "not", []() {
		assert(state.stack_len >= 1);
		stack_push(~stack_pop());
	} },
	{ "true", []() {
		assert(state.stack_len < STACK_SIZE);
		stack_push(~0);
	} },
	{ "false", []() {
		assert(state.stack_len < STACK_SIZE);
		stack_push(0);
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
	{ "hex", []() {
		assert(state.stack_len < STACK_SIZE);
		get_word();
		assert(state.interp.curr_word_len != 0);
		assert(state.interp.curr_word_len <= 8);
		uint32_t num = 0;
		for (size_t i = 0; i < state.interp.curr_word_len; ++i) {
			const char ch = state.line[state.interp.curr_word_pos+i];
			assert(('0' <= ch && ch <= '9') || ('a' <= ch && ch <= 'z') || ('A' <= ch && ch <= 'Z'));
			num <<= 4;
			if ('0' <= ch && ch <= '9') {
				num |= ch-'0';
			} else if ('a' <= ch && ch <= 'z') {
				num |= ch-'a';
			} else {
				num |= ch-'A';
			}
		}
		stack_push(num);
	} },
	{ "'", []() {
		assert(state.stack_len < STACK_SIZE);
		get_word();
		assert(state.interp.curr_word_len != 0);
		assert(state.interp.curr_word_len <= 4);
		uint32_t num = 0;
		size_t i = state.interp.curr_word_len;
		while (i --> 0) {
			const char ch = state.line[state.interp.curr_word_pos+i];
			num <<= 8;
			num |= *(const uint8_t*)&ch;
		}
		stack_push(num);
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

void get_word() {
	size_t &start = state.interp.line_pos;
	size_t word_end = 0;

	while (start < state.line_len && state.line[start] == ' ') {
		++start;
	}
	word_end = start;
	while (word_end < state.line_len && state.line[word_end] != ' ') {
		++word_end;
	}

	state.interp.curr_word_len = word_end - start;
	state.interp.curr_word_pos = start;
	start += state.interp.curr_word_len;
}

void interpret_line() {
	state.interp = {};

	size_t &word = state.interp.curr_word_pos;
	size_t &len = state.interp.curr_word_len;

	while (word < state.line_len) {
		get_word();
		if (len == 0) break;

		bool was_primitive = false;
		for (size_t pi = 0; pi < primitives_len; ++pi) {
			if (strlen(primitives[pi].name) != len) continue;

			bool matches = true;
			for (size_t i = 0; i < len; ++i) {
				if (primitives[pi].name[i] != state.line[word+i]) {
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
			for (size_t i = 0; i < len; ++i) {
				const char ch = state.line[word+i];
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
