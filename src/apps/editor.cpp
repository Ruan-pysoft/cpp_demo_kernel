#include "apps/editor.hpp"

#include <stdlib.h>

#include <sdk/util.hpp>

#include "ps2.hpp"
#include "vga.hpp"

namespace editor {

namespace {

using namespace sdk::util;

struct State {
	bool should_quit = false;
	bool capslock = false;
};

struct File {
	struct Cursor {
		size_t line = 0;
		size_t col = 0;
	} cursor;
	List<String> lines {};
};

struct File file {};

State state;

void display_buffer() {
	const auto _ = term::Backbuffer();

	term::clear();
	term::go_to(0, 0);

	for (const String *line = file.lines.begin(); line != file.lines.end(); ++line) {
		if (line != file.lines.begin()) {
			term::putchar('\n');
		}
		line->write();
	}
}

void input_key(ps2::Key key, char ch, bool capitalise) {
	if (key == ps2::KEY_BACKSPACE) {
		if (file.lines.size() && file.lines.back().size()) {
			file.lines.back().erase(file.lines.back().end()-1);
		} else if (file.lines.size() > 1) {
			file.lines.pop_back();
		}
		return;
	} else if (key == ps2::KEY_ENTER) {
		file.lines.push_back({});
	} else {
		if (capitalise && 'a' <= ch && ch <= 'z') ch ^= 'a'^'A';
		if (file.lines.size() == 0) {
			file.lines.push_back({});
		}
		file.lines.back() += ch;
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
