#include "apps/calculator.hpp"

#include <stddef.h>
#include <stdio.h>

#include <sdk/util.hpp>

#include "ps2.hpp"
#include "vga.hpp"

namespace calculator {

template<typename T>
using Maybe = sdk::util::Maybe<T>;

namespace {

struct State {
	bool should_quit = false;
	bool capslock = false;

	char line[70] = {};
	size_t line_len = 0;
};

struct Memory {
	// persistent storage

	int prev_result = 0;
} memory {};

State state;

struct Line {
	const char *text;
	size_t len;

	void advance() {
		if (len) {
			++text;
			--len;
		}
	}

	bool next_is(char ch) {
		return len && *text == ch;
	}
	bool next_between(char a, char b) {
		return len && a <= *text && *text <= b;
	}
	bool next_numeric() {
		return next_between('0', '9');
	}
	bool next_alpha() {
		return next_between('a', 'z') || next_between('A', 'Z');
	}

	void skip_space() {
		while (len && *text == ' ') {
			advance();
		}
	}
};

bool errored = false;

#define write_error(msg) do { \
		term::writestring(msg "\n"); \
		term::writestring("Here: "); \
		term::write(line.text, line.len); \
		term::putchar('\n'); \
	} while (0)
#define error(msg) do { \
		write_error(msg); \
		line = initial; \
		errored = true; \
		return {}; \
	} while (0)

Maybe<int> parse_num(Line &line) {
	const auto initial = line;

	line.skip_space();

	const bool neg = line.next_is('-');
	if (neg) {
		line.advance();
	}

	if (!line.next_numeric()) {
		line = initial;
		return {};
	}

	uint32_t num = 0;
	while (line.next_numeric()) {
		const uint32_t old_num = num;
		num *= 10;
		num += *line.text - '0';

		line.advance();

		// I'm not sure if this overflow logic is correct?
		if (num < old_num || ((num & (1u<<31)) && !(num == (1u<<31) && neg))) {
			error("integer overflow");
		}
	}

	if (neg) num = (~num)+1;

	return *(int*)&num;
}

Maybe<int> eval_expr(Line &line);
Maybe<int> eval_parenthesised(Line &line) {
	const auto initial = line;

	line.skip_space();
	if (line.next_is('(')) {
		line.advance();
		const auto res = eval_expr(line);
		if (errored) {
			line = initial;
			return {};
		}
		if (!res.has) {
			error("expected expression");
		}

		line.skip_space();
		if (line.next_is(')')) {
			line.advance();
			return res.get();
		} else {
			error("expected closing parenthesis");
		}
	} else {
		return parse_num(line);
	}
}

Maybe<int> eval_pre_unary(Line &line) {
	// TODO: handle overflow?

	const auto initial = line;

	bool negate = false;

	line.skip_space();
	while (line.next_is('+') || line.next_is('-')) {
		while (line.next_is('+')) {
			line.advance();
			line.skip_space();
		}

		const auto num = eval_parenthesised(line);
		if (errored) {
			line = initial;
			return {};
		}
		if (num.has) {
			return negate ? -num.get() : num.get();
		}

		if (line.next_is('-')) {
			negate = !negate;
			line.advance();
			line.skip_space();
		}
	}

	const auto num = eval_parenthesised(line);
	if (errored) {
		line = initial;
		return {};
	}
	if (num.has) {
		return negate ? -num.get() : num.get();
	}

	error("expected expression");
}

int ipow(int a, int b) {
	if (b < 0) return 1 / ipow(a, -b);
	int res = 1;
	// TODO: binary exponentiation?
	for (int i = 0; i < b; ++i) {
		res *= a;
	}
	return res;
}

Maybe<int> eval_pow(Line &line) {
	const auto initial = line;

	int res = 0;

	auto num = eval_pre_unary(line);
	if (errored) {
		line = initial;
		return {};
	}
	if (!num.has) {
		error("expected expression");
	}
	res = num.get();

	line.skip_space();
	while (line.next_is('^')) {
		line.advance();

		num = eval_pre_unary(line);
		if (errored) {
			line = initial;
			return {};
		}
		if (!num.has) {
			error("expected expression");
		}

		res = ipow(res, num.get());
	}

	return res;
}

Maybe<int> eval_muldiv(Line &line) {
	const auto initial = line;

	int res = 0;

	auto num = eval_pow(line);
	if (errored) {
		line = initial;
		return {};
	}
	if (!num.has) {
		error("expected expression");
	}
	res = num.get();

	line.skip_space();
	while (line.next_is('*') || line.next_is('/') || line.next_is('%')) {
		const char op = *line.text;
		line.advance();

		num = eval_pow(line);
		if (errored) {
			line = initial;
			return {};
		}
		if (!num.has) {
			error("expected expression");
		}

		if (op == '*') {
			res *= num.get();
		} else if (op == '/') {
			res /= num.get();
		} else {
			res %= num.get();
		}

		line.skip_space();
	}

	return res;
}

Maybe<int> eval_term(Line &line) {
	// TODO: handle overflow?

	const auto initial = line;

	int res = 0;

	auto num = eval_muldiv(line);
	if (errored) {
		line = initial;
		return {};
	}
	if (!num.has) {
		error("expected expression");
	}
	res = num.get();

	line.skip_space();
	while (line.next_is('+') || line.next_is('-')) {
		const char op = *line.text;
		line.advance();

		num = eval_muldiv(line);
		if (errored) {
			line = initial;
			return {};
		}
		if (!num.has) {
			error("expected expression");
		}

		if (op == '+') {
			res += num.get();
		} else {
			res -= num.get();
		}

		line.skip_space();
	}

	return res;
}

Maybe<int> eval_expr(Line &line) {
	return eval_term(line);
}

void eval_line() {
	errored = false;
	Line line = {
		state.line,
		state.line_len,
	};
	state.line_len = 0;

	line.skip_space();

	auto result = eval_expr(line);
	if (errored) return;

	if (result.has) {
		printf("%d\n", result.get());
		memory.prev_result = result.get();
	} else {
		term::writestring("Please enter an expression!\n");
		return;
	}

	line.skip_space();
	if (line.len) {
		write_error("extraneous text after expression");
		return;
	}
}

void input_key(ps2::Key key, char ch, bool capitalise) {
	if (state.line_len == sizeof(state.line)) {
		return;
	}

	if (key == ps2::KEY_BACKSPACE) {
		if (state.line_len) --state.line_len;
		term::backspace();
		return;
	} else if (key == ps2::KEY_ENTER) {
		term::putchar('\n');
		eval_line();
	} else {
		if (capitalise && 'a' <= ch && ch <= 'z') ch ^= 'a'^'A';
		state.line[state.line_len++] = ch;
		term::putchar(ch);
	}
}

void handle_keyevent(ps2::EventType type, ps2::Key key) {
	using namespace ps2;

	if (type != EventType::Press && type != EventType::Bounce) return;

	if (key_ascii_map[key] || key == KEY_BACKSPACE) {
		const bool capitalise = key_state[KEY_LSHIFT]
			|| key_state[KEY_RSHIFT];
		input_key(key, key_ascii_map[key], capitalise ^ state.capslock);
		return;
	}

	if (key == KEY_CAPSLOCK) {
		state.capslock = !state.capslock;
	}

	if (key == KEY_ESCAPE) {
		state.should_quit = true;
	}
}

}

void main() {
	state = State{};

	term::clear();
	term::go_to(0, 0);

	while (!state.should_quit) {
		const bool had_events = !ps2::events.empty();

		while (!ps2::events.empty()) {
			const auto event = ps2::events.pop();
			handle_keyevent(event.type, event.key);
		}

		__asm__ volatile("hlt" ::: "memory");
	}
}

}
