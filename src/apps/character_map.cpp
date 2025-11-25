#include "apps/character_map.hpp"

#include <stdint.h>
#include <string.h>

#include "ps2.hpp"
#include "vga.hpp"

using namespace term;

namespace character_map {

namespace {

struct State {
	uint8_t pos = 0;
	bool search_mode = false;
};

void draw(State &state) {
	auto _ = term::Backbuffer();

	const char *title = "CHARACTER SET";
	const size_t title_len = strlen(title);
	const size_t title_offset = (vga::WIDTH - title_len)/2;

	go_to(title_offset, 1);
	setcolor(vga::entry_color(vga::Color::Black, vga::Color::LightGrey));
	writestring(title);
	resetcolor();

	const size_t line_width = 0x10;
	const size_t line_offset = (vga::WIDTH - line_width)/2;

	go_to(line_offset, 3);
	for (uint8_t low_half = 0; low_half <= 0xF; ++low_half) {
		if (low_half == (state.pos&0xF)) setcolor(vga::entry_color(
			vga::Color::Black,
			vga::Color::LightGrey
		));
		if (low_half < 0xA) {
			putchar('0'+low_half);
		} else {
			putchar('a'+low_half-0xA);
		}
		if (low_half == (state.pos&0xF)) resetcolor();
	}

	for (uint8_t high_half = 0; high_half <= 0xF; ++high_half) {
		go_to(line_offset - 3, 5 + high_half);

		if (high_half == (state.pos>>4)) setcolor(vga::entry_color(
			vga::Color::Black,
			vga::Color::LightGrey
		));
		if (high_half < 0xA) {
			putchar('0'+high_half);
		} else {
			putchar('a'+high_half-0xA);
		}
		if (high_half == (state.pos>>4)) resetcolor();
		putchar(' ');
		putchar(' ');

		for (uint8_t low_half = 0; low_half <= 0xF; ++low_half) {
			const uint8_t byte = low_half + (high_half << 4);
			if (byte == state.pos) setcolor(vga::entry_color(
				vga::Color::DarkGrey,
				vga::Color::White
			));
			putbyte(byte);
			if (byte == state.pos) resetcolor();
		}
	}

	const char *help_text = "Press ESC or Q to quit \xb3 Press : and then a letter to jump to that letter";
	const size_t help_len = strlen(help_text);
	const size_t help_offset = (vga::WIDTH - help_len)/2;

	go_to(help_offset, vga::HEIGHT-1 - 1);
	if (state.search_mode) setcolor(vga::entry_color(
		vga::Color::Black,
		vga::Color::LightGrey
	));
	writestring(help_text);
	if (state.search_mode) resetcolor();
}

bool handle_keyevent(ps2::Event event, State &state) {
	using namespace ps2;

	const EventType type = event.type;
	const Key key = event.key;

	if (type != EventType::Release) {
		if (state.search_mode && type == EventType::Press && key_ascii_map[key]) {
			char ch = key_ascii_map[key];

			if (key_state[KEY_LSHIFT] || key_state[KEY_RSHIFT]) {
				if ('a' <= ch && ch <= 'z') {
					// capitalise alphabetic key
					ch ^= 'a'^'A';
				}
			}

			state.pos = ch;
			state.search_mode = false;
			draw(state);

			return false;
		}

		switch (key) {
			using namespace ps2;

			case KEY_ESCAPE: {
				if (state.search_mode) {
					state.search_mode = false;
					return false;
				}
				return true;
			} break;
			case KEY_Q: {
				return true;
			} break;

			case KEY_UP: {
				state.pos -= 0x10;
				state.search_mode = false;
				draw(state);
			} break;
			case KEY_DOWN: {
				state.pos += 0x10;
				state.search_mode = false;
				draw(state);
			} break;
			case KEY_LEFT: {
				state.pos -= 0x01;
				state.search_mode = false;
				draw(state);
			} break;
			case KEY_RIGHT: {
				state.pos += 0x01;
				state.search_mode = false;
				draw(state);
			} break;

			case KEY_COLON: {
				state.search_mode = true;
				draw(state);
				return false;
			} break;

			default: break;
		}
	}

	return false;
}

}

void main() {
	State state{};

	draw(state);

	for (;;) {
		__asm__ volatile("hlt" ::: "memory");

		while (!ps2::events.empty()) {
			if (handle_keyevent(ps2::events.pop(), state)) return;
		}
	}
}

}
