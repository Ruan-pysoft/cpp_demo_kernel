#include "apps/editor.hpp"

#include <stdlib.h>

#include "ps2.hpp"
#include "vga.hpp"

namespace editor {

namespace {

struct State {
	bool should_quit = false;
	bool capslock = false;
};

char *buffer = nullptr;
size_t buffer_len = 0;
size_t buffer_size = 0;

State state;

void display_buffer() {
	const auto _ = term::Backbuffer();

	term::clear();
	term::go_to(0, 0);

	for (size_t i = 0; i < buffer_len; ++i) {
		term::putchar(buffer[i]);
	}
}

void input_key(ps2::Key key, char ch, bool capitalise) {
	if (buffer_len == buffer_size) {
		const size_t new_size = buffer_size == 0 ? 1024 : buffer_size * 2;
		buffer = (char*)realloc(buffer, new_size);
	}

	if (key == ps2::KEY_BACKSPACE) {
		if (buffer_len) --buffer_len;
		return;
	} else {
		if (capitalise && 'a' <= ch && ch <= 'z') ch ^= 'a'^'A';
		buffer[buffer_len++] = ch;
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

	display_buffer();

	while (!state.should_quit) {
		const bool had_events = !ps2::events.empty();

		while (!ps2::events.empty()) {
			const auto event = ps2::events.pop();
			handle_keyevent(event.type, event.key);
		}

		if (had_events) display_buffer();

		__asm__ volatile("hlt" ::: "memory");
	}
}

}
