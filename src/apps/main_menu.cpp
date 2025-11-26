#include "apps/main_menu.hpp"

#include <stddef.h>
#include <string.h>

#include "ps2.hpp"
#include "vga.hpp"

#include "apps/snake.hpp"
#include "apps/forth.hpp"
#include "apps/character_map.hpp"
#include "apps/queued_demo.hpp"
#include "apps/callback_demo.hpp"
#include "apps/ignore_demo.hpp"

using namespace term;

namespace main_menu {

namespace {

constexpr struct MenuEntry {
	const char *name;
	void (*main)(void);
} menu_entries[] = {
	{ "Snake game", snake::main },
	{ "FORTH interpreter", forth::main },
	{ "Display character map", character_map::main },
	{ "EventLoop Demo: QueuedEventLoop", queued_demo::main },
	{ "EventLoop Demo: CallbackEventLoop", callback_demo::main },
	{ "EventLoop Demo: IgnoreEventLoop", ignore_demo::main },
};
constexpr size_t menu_entries_len = sizeof(menu_entries)/sizeof(*menu_entries);

struct State {
	size_t index = 0;

	void redraw() {
		auto _ = Backbuffer();

		cursor::disable();
		resetcolor();
		clear();
		go_to(0, 0);

		const char *title = "AVAILABLE APPLICATIONS";
		const size_t title_len = strlen(title);
		const size_t title_offset = (vga::WIDTH - title_len) / 2;

		go_to(title_offset, 1);
		setcolor(vga::entry_color(vga::Color::Black, vga::Color::LightGrey));
		writestring(title);
		resetcolor();

		for (size_t i = 0; i < menu_entries_len; ++i) {
			go_to(1, 3 + i);
			putchar('-');

			if (i == index) {
				setcolor(vga::entry_color(vga::Color::Blue, vga::Color::White));
			}

			go_to(3, 3 + i);
			writestring(menu_entries[i].name);

			if (i == index) {
				resetcolor();
			}
		}
	}

	void tick() {
		bool should_redraw = false;

		while (!ps2::events.empty()) {
			should_redraw = true;

			handle_key(ps2::events.pop());
		}

		if (should_redraw) redraw();
	}

	void handle_key(ps2::Event key) {
		if (key.type == ps2::EventType::Release) return;

		switch (key.key) {
			case ps2::KEY_UP:
			case ps2::KEY_LEFT:
			case ps2::KEY_W:
			case ps2::KEY_A:
			case ps2::KEY_K:
			case ps2::KEY_H: {
				if (index > 0) --index;
			} break;
			case ps2::KEY_DOWN:
			case ps2::KEY_RIGHT:
			case ps2::KEY_D:
			case ps2::KEY_S:
			case ps2::KEY_J:
			case ps2::KEY_L: {
				if (index < menu_entries_len-1) ++index;
			} break;
			case ps2::KEY_ENTER:
			case ps2::KEY_SPACE: {
				cursor::enable(8, 15);
				resetcolor();
				clear();
				go_to(0, 0);
				menu_entries[index].main();
			} break;
			default: break;
		}
	}
};

}

void main() {
	State state{};

	state.redraw();

	for (;;) {
		__asm__ volatile("hlt" ::: "memory");

		state.tick();
	}
}

}
