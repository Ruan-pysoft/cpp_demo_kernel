#include "apps/components/menu.hpp"

#include <string.h>

#include <sdk/terminal.hpp>

#include "ps2.hpp"
#include "vga.hpp"

Menu::Menu(const MenuEntry *entries, size_t entries_len,
	   const MenuEntry *hidden, size_t hidden_len, const char *title,
	   const char *title_hidden)
: title(title), title_hidden(title_hidden), entries(entries),
	entries_len(entries_len), hidden(hidden), hidden_len(hidden_len) {
	if (title_hidden == nullptr) title_hidden = title;
	title_len = strlen(title);
	title_hidden_len = strlen(title_hidden);
}

void Menu::draw() const {
	using namespace term;

	cursor::disable();
	auto _ = Backbuffer();

	/* Set up consistent starting state */
	resetcolor();
	clear();
	go_to(0, 0);

	if (show_hidden) {
		const size_t offset = (vga::WIDTH - title_hidden_len)/2;

		go_to(offset, 1);
		sdk::colors::with(
			vga::Color::Black, vga::Color::LightRed,
			writestring, title_hidden
		);
	} else {
		const size_t offset = (vga::WIDTH - title_len)/2;

		go_to(offset, 1);
		sdk::colors::with(
			vga::Color::Black, vga::Color::LightGrey,
			writestring, title
		);
	}

	for (size_t i = 0; i < entries_len; ++i) {
		go_to(1, 3 + i);
		putchar('-');

		auto colors = sdk::ColorSwitch();

		if (i == index) {
			colors.set(vga::Color::Blue, vga::Color::White);
		}

		go_to(3, 3 + i);
		writestring(entries[i].name);
	}

	if (show_hidden) {
		for (size_t i = 0; i < hidden_len; ++i) {
			go_to(1, 3 + entries_len + i);
			putchar('-');

			auto colors = sdk::ColorSwitch();

			if (entries_len + i == index) {
				colors.set(vga::Color::Black, vga::Color::LightBlue);
			}

			go_to(3, 3 + entries_len + i);
			writestring(hidden[i].name);
		}
	}
}
void Menu::handle_key(ps2::Event key) {
	using namespace ps2;

	if (key.type == EventType::Release) return;

	switch (key.key) {
		case KEY_UP:
		case KEY_LEFT:
		case KEY_W:
		case KEY_A:
		case KEY_K:
		case KEY_H: {
			prev();
		} break;
		case KEY_DOWN:
		case KEY_RIGHT:
		case KEY_S:
		case KEY_D:
		case KEY_J:
		case KEY_L: {
			next();
		} break;
		case KEY_ENTER:
		case KEY_SPACE: {
			select();
		} break;
		case KEY_COLON: {
			toggle_hidden();
		} break;
		default: break;
	}
}
void Menu::handle_key_of(Menu &menu, ps2::Event key) { menu.handle_key(key); }

void Menu::prev() {
	if (index > 0) --index;
	else if (show_hidden) index = entries_len+hidden_len - 1;
	else index = entries_len - 1;
}
void Menu::next() {
	if (show_hidden && index < entries_len+hidden_len - 1) ++index;
	else if (!show_hidden && index < entries_len - 1) ++index;
	else index = 0;
}

void Menu::hide() {
	show_hidden = false;
	if (index >= entries_len) index = entries_len-1;
}
void Menu::show() {
	show_hidden = true;
}
void Menu::toggle_hidden() {
	show_hidden = !show_hidden;
	if (!show_hidden && index >= entries_len) index = entries_len-1;
}

const MenuEntry &Menu::curr() const {
	if (index < entries_len) return entries[index];
	else return hidden[index - entries_len];
}

void Menu::select() const {
	term::cursor::enable();
	term::resetcolor();
	term::clear();
	term::go_to(0, 0);
	curr().func();
	draw();
}
