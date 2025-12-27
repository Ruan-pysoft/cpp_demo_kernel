#pragma once

#include <stddef.h>
#include <string.h>

#include <sdk/eventloop.hpp>
#include <sdk/terminal.hpp>
#include <sdk/util.hpp>

#include "ps2.hpp"
#include "vga.hpp"

namespace menu {

using namespace sdk::util;

template<typename T>
struct Entry {
	const char *name;
	void (*fun)(T);
	T arg;
};

template<typename T>
class Menu {
	const List<Entry<T>> &entries;
	const List<Entry<T>> &entries_hidden;
	String title;
	String title_hidden;

	bool show_hidden = false;
	size_t index = 0;
public:
	Menu(const List<Entry<T>> &entries,
		const List<Entry<T>> &entries_hidden, const String &title,
		const String &title_hidden)
	: entries(entries), entries_hidden(entries_hidden), title(title), title_hidden(title_hidden)
	{ }
	Menu(const List<Entry<T>> &entries,
		const List<Entry<T>> &entries_hidden, const String &title)
	: Menu(entries, entries_hidden, title, title)
	{ }

	void draw() const {
		using namespace term;

		cursor::disable();
		auto _ = Backbuffer();

		/* Set up consistent starting state */
		resetcolor();
		clear();
		go_to(0, 0);

		if (show_hidden) {
			const size_t offset = (vga::WIDTH - title_hidden.size())/2;

			go_to(offset, 1);
			sdk::colors::with(
				vga::Color::Black, vga::Color::LightRed,
				writestring, title_hidden.c_str()
			);
		} else {
			const size_t offset = (vga::WIDTH - title.size())/2;

			go_to(offset, 1);
			sdk::colors::with(
				vga::Color::Black, vga::Color::LightGrey,
				writestring, title.c_str()
			);
		}

		for (size_t i = 0; i < entries.size(); ++i) {
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
			for (size_t i = 0; i < entries_hidden.size(); ++i) {
				go_to(1, 3 + entries.size() + i);
				putchar('-');

				auto colors = sdk::ColorSwitch();

				if (entries.size() + i == index) {
					colors.set(vga::Color::Black, vga::Color::LightBlue);
				}

				go_to(3, 3 + entries.size() + i);
				writestring(entries_hidden[i].name);
			}
		}
	}
	void handle_key(ps2::Event key) {
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
	static void handle_key_of(Menu &menu, ps2::Event key) { menu.handle_key(key); }

	void prev() {
		if (index > 0) --index;
		else if (show_hidden) {
			index = entries.size()+entries_hidden.size() - 1;
		} else index = entries.size() - 1;
	}
	void next() {
		if (show_hidden && index < entries.size()+entries_hidden.size() - 1) ++index;
		else if (!show_hidden && index < entries.size() - 1) ++index;
		else index = 0;
	}

	inline bool is_hidden() const { return show_hidden; }
	void hide() {
		show_hidden = false;
		if (index >= entries.size()) index = entries.size()-1;
	}
	void show() {
		show_hidden = true;
	}
	void toggle_hidden() {
		show_hidden = !show_hidden;
		if (!show_hidden && index >= entries.size()) index = entries.size()-1;
	}

	const Entry<T> &curr() const {
		if (index < entries.size()) return entries[index];
		else return entries_hidden[index - entries.size()];
	}

	void select() const {
		term::cursor::enable();
		term::resetcolor();
		term::clear();
		term::go_to(0, 0);
		curr().fun(curr().arg);
	}
};

}
