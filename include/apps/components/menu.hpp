#pragma once

#include <stddef.h>

#include <sdk/eventloop.hpp>

#include "ps2.hpp"

struct MenuEntry {
	const char *name;
	void (*func)();
};

class Menu {
	const char *title;
	size_t title_len;
	const char *title_hidden;
	size_t title_hidden_len;
	const MenuEntry *entries;
	size_t entries_len;
	const MenuEntry *hidden;
	size_t hidden_len;

	bool show_hidden = false;
	size_t index = 0;
public:
	Menu(const MenuEntry *entries, size_t entries_len,
		const MenuEntry *hidden, size_t hidden_len, const char *title,
		const char *title_hidden = nullptr);

	void draw() const;
	void handle_key(ps2::Event key);
	static void handle_key_of(Menu &menu, ps2::Event key);

	void prev();
	void next();

	inline bool is_hidden() const { return show_hidden; }
	void hide();
	void show();
	void toggle_hidden();

	const MenuEntry &curr() const;

	void select() const;
};
