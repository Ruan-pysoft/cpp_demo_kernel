#include "apps/main_menu.hpp"

#include <stddef.h>
#include <string.h>

#include <sdk/terminal.hpp>

#include "ps2.hpp"
#include "vga.hpp"

#include "apps/components/menu.hpp"

#include "apps/snake.hpp"
#include "apps/forth.hpp"
#include "apps/character_map.hpp"
#include "apps/queued_demo.hpp"
#include "apps/callback_demo.hpp"
#include "apps/ignore_demo.hpp"

using namespace term;

namespace main_menu {

namespace {

constexpr MenuEntry menu_entries[] = {
	{ "Snake game", snake::main },
	{ "FORTH interpreter", forth::main },
	{ "Display character map", character_map::main },
};
constexpr size_t menu_entries_len = sizeof(menu_entries)/sizeof(*menu_entries);
constexpr MenuEntry hidden_menu_entries[] = {
	{ "DEBUG: QueuedEventLoop Demo", queued_demo::main },
	{ "DEBUG: CallbackEventLoop Demo", callback_demo::main },
	{ "DEBUG: IgnoreEventLoop Demo", ignore_demo::main },
};
constexpr size_t hidden_menu_entries_len = sizeof(hidden_menu_entries)/sizeof(*hidden_menu_entries);

}

void main() {
	Menu menu(
		menu_entries, menu_entries_len,
		hidden_menu_entries, hidden_menu_entries_len,
		"AVAILABLE APPLICATIONS",
		"AVAILABLE APPLICATIONS - ADVANCED"
	);

	menu.draw();

	for (;;) {
		__asm__ volatile("hlt" ::: "memory");

		bool should_redraw = false;
		while (!ps2::events.empty())  {
			should_redraw = true;

			menu.handle_key(ps2::events.pop());
		}
		if (should_redraw) menu.draw();
	}
}

}
