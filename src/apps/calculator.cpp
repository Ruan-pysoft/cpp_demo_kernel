#include "apps/calculator.hpp"

#include <stddef.h>
#include <stdio.h>

#include "ps2.hpp"
#include "vga.hpp"

namespace calculator {

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

void eval_line() {
	const char *line = state.line;
	size_t line_len = state.line_len;
	state.line_len = 0;

	while (line_len && *line == ' ') {
		++line;
		--line_len;
	}

	if (line_len && *line == '_') {
		++line;
		--line_len;

		while (line_len && *line == ' ') {
			++line;
			--line_len;
		}

		if (line_len) {
			term::writestring("ERROR: Extraneous characters after number!\n");
			return;
		}

		printf("%d\n", memory.prev_result);

		return;
	}

	const bool neg = line_len && *line == '-';
	if (neg) {
		++line;
		--line_len;
	}

	if (!line_len) {
		term::writestring("ERROR: Please enter a number!\n");
		return;
	}

	uint32_t num = 0;
	while (line_len && '0' <= *line && *line <= '9') {
		const uint32_t old_num = num;
		num *= 10;
		num += *line - '0';

		++line;
		--line_len;

		// I'm not sure if this overflow logic is correct?
		if (num < old_num || ((num & (1u<<31)) && !(num == (1u<<31) && neg))) {
			term::writestring("ERROR: Integer overflow!\n");
			return;
		}
	}

	while (line_len && *line == ' ') {
		++line;
		--line_len;
	}

	if (line_len) {
		term::writestring("ERROR: Extraneous characters after number!\n");
		return;
	}

	if (neg) num = (~num)+1;
	int inum = *(int*)&num;

	printf("%d\n", inum);
	memory.prev_result = inum;
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
