#include "apps/forth.hpp"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>

#include "blit.hpp"

#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace forth {

namespace {

constexpr size_t MAX_LINE_LEN = vga::WIDTH - 3;
char line[MAX_LINE_LEN];

struct State {
	bool should_quit = false;
	bool capslock = false;
	size_t line_len = 0;
	bool has_err = false;
	uint32_t err_until = 0;
};
static State state;
bool forth_running = false;

void input_key(ps2::Key, char ch, bool capitalise) {
	if (state.line_len == MAX_LINE_LEN) {
		state.has_err = true;
		state.err_until = pit::millis + 100;

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
	line[state.line_len++] = ch;
	putchar(ch);

	if (state.line_len == MAX_LINE_LEN) term::cursor::disable();
}

void interpret_line() {
	if (state.line_len == 4 && line[0] == 'q' && line[1] == 'u' && line[2] == 'i' && line[3] == 't') {
		state.should_quit = true;
	}
	if (state.line_len == 4 && line[0] == 'e' && line[1] == 'x' && line[2] == 'i' && line[3] == 't') {
		state.should_quit = true;
	}

	for (size_t i = 0; i < state.line_len; ++i) {
		putchar(line[i]);
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

		if (state.has_err) {
			state.has_err = false;

			term::advance();
			term::backspace();
		}

		--state.line_len;
		term::backspace();
		term::cursor::enable(8, 15);
	}

	if (key == KEY_ENTER) {
		if (state.has_err) {
			state.has_err = false;

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

		if (state.has_err && pit::millis >= state.err_until) {
			state.has_err = false;

			term::advance();
			term::backspace();
		}

		__asm__ volatile("hlt" ::: "memory");
	}

	forth_running = false;
}

}
