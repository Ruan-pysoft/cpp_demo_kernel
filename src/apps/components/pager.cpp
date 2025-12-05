#include "apps/components/pager.hpp"

#include <string.h>

#include <sdk/terminal.hpp>

#include "ps2.hpp"
#include "vga.hpp"

using namespace sdk::util;

static constexpr size_t PAGER_HEIGHT = vga::HEIGHT-1;

Pager::Pager(const char *text) : text(text), lines() {
	const size_t text_len = strlen(text);
	size_t line_begin = 0;
	size_t i;
	for (i = 0; i < text_len; ++i) {
		if (text[i] == '\n') {
			const size_t line_len = i - line_begin;
			const size_t line_span = line_len/vga::WIDTH + !!(line_len%vga::WIDTH);
			lines.push_back({ line_begin, line_len, line_span });
			++i; // skip \n
			while (i < text_len && text[i] == '\n') {
				lines.push_back({ i, 0, 1 });
				++i;
			}
			line_begin = i;
		}
	}
	const size_t line_len = text_len - line_begin;
	const size_t line_span = line_len/vga::WIDTH + !!(line_len%vga::WIDTH);
	if (line_len) lines.push_back({ line_begin, line_len, line_span });
}

void Pager::draw() const {
	const auto _ = term::Backbuffer();

	term::clear();
	term::go_to(0, 0);
	term::disable_autoscroll();

	size_t sub_lines_to_skip = sub_line;
	size_t line_idx = top_line;
	size_t visual_lines_to_write = PAGER_HEIGHT;

	while (visual_lines_to_write) {
		if (line_idx >= lines.size()) {
			const auto _ = sdk::ColorSwitch(
				vga::Color::DarkGrey,
				vga::Color::Black
			);

			for (size_t i = 0; i < visual_lines_to_write; ++i) {
				for (size_t j = 0; j < vga::WIDTH; ++j) {
					term::putbyte(0xf9);
				}
			}

			break;
		}

		const auto &line = lines[line_idx];
		if (line.term_span <= sub_lines_to_skip) {
			sub_lines_to_skip -= line.term_span;
			continue;
		}
		const size_t term_span = line.term_span - sub_lines_to_skip;
		const size_t to_write = term_span > visual_lines_to_write
			? visual_lines_to_write - 1
			: term_span;

		for (size_t i = 0; i < to_write; ++i) {
			const size_t idx = i + sub_lines_to_skip;
			if (idx == line.term_span-1 && (line.len%vga::WIDTH || line.len == 0)) {
				term::write(&text[line.pos + idx*vga::WIDTH], line.len%vga::WIDTH);
				term::putchar('\n');
			} else {
				term::write(&text[line.pos + idx*vga::WIDTH], vga::WIDTH);
			}
		}

		if (term_span > visual_lines_to_write) {
			const auto _ = sdk::ColorSwitch(
				vga::Color::Black,
				vga::Color::LightGrey
			);
			term::writestring("...\n");
			break;
		}

		++line_idx;
		visual_lines_to_write -= to_write;
		sub_lines_to_skip = 0;
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
	if (sub_line < lines[top_line].term_span-1) ++sub_line;
	else if (top_line < lines.size()-1) {
		sub_line = 0;
		++top_line;
	}
}
void Pager::move_up_visual() {
	if (sub_line > 0) --sub_line;
	else if (top_line > 0) {
		--top_line;
		sub_line = lines[top_line].term_span-1;
	}
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
	for (size_t i = 0; i < PAGER_HEIGHT-1; ++i) {
		move_down_visual();
	}
}
void Pager::page_up() {
	for (size_t i = 0; i < PAGER_HEIGHT-1; ++i) {
		move_up_visual();
	}
}
void Pager::home() {
	top_line = 0;
	sub_line = 0;
}
void Pager::end() {
	top_line = lines.size()-1;
	sub_line = 0;
}
