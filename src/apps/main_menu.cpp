#include "apps/main_menu.hpp"

#include <stddef.h>
#include <string.h>

#include <sdk/terminal.hpp>

#include "ps2.hpp"
#include "vga.hpp"

#include "apps/components/menu.hpp"
#include "apps/components/pager.hpp"

#include "apps/snake.hpp"
#include "apps/forth.hpp"
#include "apps/character_map.hpp"
#include "apps/queued_demo.hpp"
#include "apps/callback_demo.hpp"
#include "apps/ignore_demo.hpp"

using namespace term;

namespace main_menu {

namespace {

void pager_test() {
	const char *text =
		"This is a long text. And I should have at least a few multi-line paragraphs in here. As well as empty lines, obviously. The purpose of this text is to test the \"pager\" component to see how well it functions. (Or well, to see *that* it functions)\n"
		"\n"
		"\n"
		"The quick brown fox jumps over the lazy dog.\n"
		"The quick brown fox jumps over the lazy dog.\n"
		"Lorum ipsum dolor set [...]\n"
		"Never gonna give you up, never gonna let you down, never gonna run around and desert you.\n"
		"Never gonna remember the rest of these lyrics either.\n"
		"\n"
		"\"Jy moet die Here jou God liefh\x88 met jou hele hart en met jou hele siel en met jou hele verstand.\" Dit is die belangrikste gebod. En die tweede wat hieraan gelyk staan, is, \"Jy moet jou naaste liefh\x88 soos jouself.\"\n"
	;

	auto pager = Pager(text);
	pager.draw();

	while (!pager.should_quit) {
		__asm__ volatile("hlt" ::: "memory");

		bool should_redraw = false;
		while (!ps2::events.empty()) {
			should_redraw = true;

			pager.handle_key(ps2::events.pop());
		}
		if (should_redraw) pager.draw();
	}
}

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
	{ "DEBUG: Pager Test", pager_test },
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
