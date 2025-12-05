#include "apps/components/pager.hpp"

#include <string.h>

#include <sdk/terminal.hpp>

#include "ps2.hpp"
#include "vga.hpp"

using namespace sdk::util;

Pager::Pager(const char *text) : text(text), lines() {
	const size_t text_len = strlen(text);
	size_t line_begin = 0;
	size_t i;
	for (i = 0; i < text_len; ++i) {
		if (text[i] == '\n') {
			const size_t line_len = i - line_begin;
			lines.push_back({ line_begin, line_len });
			++i; // skip \n
			while (i < text_len && text[i] == '\n') {
				lines.push_back({ i, 0 });
				++i;
			}
			line_begin = i;
		}
	}
	const size_t line_len = text_len - line_begin;
	if (line_len) lines.push_back({ line_begin, line_len });
}

void Pager::draw() const {
	const auto _ = term::Backbuffer();

	term::clear();
	term::go_to(0, 0);
	term::disable_autoscroll();

	size_t line_idx = top_line;
	size_t visual_lines_to_write = vga::HEIGHT - 1;

	while (visual_lines_to_write) {
		if (line_idx >= lines.size()) {
			const auto _ = sdk::ColorSwitch(
				vga::Color::DarkGrey,
				vga::Color::Black
			);

			for (size_t i = 0; i < visual_lines_to_write; ++i) {
				term::writestring("\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9\xf9");
			}

			break;
		}

		const auto &line_pos = lines[line_idx].first;
		const auto &line_len = lines[line_idx].second;
		const size_t line_span = line_len == 0 ? 1 : line_len/vga::WIDTH + !!(line_len%vga::WIDTH);
		const size_t to_write = line_span > visual_lines_to_write ? visual_lines_to_write - 1 : line_span;

		for (size_t i = 0; i < to_write; ++i) {
			if (i == line_span-1 && (line_len%vga::WIDTH || line_len == 0)) {
				term::write(&text[line_pos + i*vga::WIDTH], line_len%vga::WIDTH);
				term::putchar('\n');
			} else {
				term::write(&text[line_pos + i*vga::WIDTH], vga::WIDTH);
			}
		}

		if (line_span > visual_lines_to_write) {
			const auto _ = sdk::ColorSwitch(
				vga::Color::Black,
				vga::Color::LightGrey
			);
			term::writestring("...\n");
			break;
		}

		++line_idx;
		visual_lines_to_write -= to_write;
	}

	term::enable_autoscroll();
}
void Pager::handle_key(ps2::Event key) {
	using namespace ps2;

	if (key.type == EventType::Release) return;

	switch (key.key) {
		case KEY_UP:
		case KEY_W:
		case KEY_K: {
			if (key_state[KEY_LSHIFT] || key_state[KEY_RSHIFT]) {
				move_up_line();
			} else {
				move_up_visual();
			}
		} break;
		case KEY_DOWN:
		case KEY_S:
		case KEY_J: {
			if (key_state[KEY_LSHIFT] || key_state[KEY_RSHIFT]) {
				move_down_line();
			} else {
				move_down_visual();
			}
		} break;
		case KEY_PAGEUP: { 
			page_up();
		} break;
		case KEY_PAGEDOWN: { 
			page_down();
		} break;
		case KEY_HOME: { 
			home();
		} break;
		case KEY_END: { 
			end();
		} break;
		case KEY_Q:
		case KEY_ESCAPE: {
			should_quit = true;
		} break;
		default: break;
	}
}

void Pager::move_down_visual() {
	sub_line = 0;
	// TODO: properly implement
	if (top_line < lines.size()-1) ++top_line;
}
void Pager::move_up_visual() {
	// TODO: properly implement
	sub_line = 0;
	if (top_line > 0) --top_line;
}
void Pager::move_down_line() {
	sub_line = 0;
	if (top_line < lines.size()-1) ++top_line;
}
void Pager::move_up_line() {
	sub_line = 0;
	if (top_line > 0) --top_line;
}
void Pager::page_down() {
	assert(false && "TODO");
}
void Pager::page_up() {
	assert(false && "TODO");
}
void Pager::home() {
	top_line = 0;
	sub_line = 0;
}
void Pager::end() {
	top_line = lines.size()-1;
	sub_line = 0;
}
