#include "apps/editor.hpp"

#include <stdlib.h>

#include <sdk/util.hpp>
#include <sdk/terminal.hpp>

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
	struct Pos {
		size_t line = 0;
		size_t col = 0;
	};
	Pos cursor {};
	Pos screen_top {};
	List<String> lines {};

	void insert_character(Pos at, char c) {
		if (lines.size() == 0) {
			lines.push_back({});
		}

		assert(at.line < lines.size());
		auto &line = lines[at.line];

		assert(at.col <= line.size());

		line.insert(at.col, c);
	}
	void insert_newline(Pos at) {
		if (lines.size() == 0) {
			lines.push_back({});
		}

		assert(at.line < lines.size());
		auto &line = lines[at.line];

		assert(at.col <= line.size());

		if (at.col == line.size()) {
			lines.insert(at.line+1, {});
		} else {
			lines.insert(at.line+1, line.substr(at.col));
			line.erase(at.col);
		}
	}
	void delete_character(Pos at) {
		if (lines.size() == 0) {
			return;
		}

		assert(at.line < lines.size());
		auto &line = lines[at.line];

		assert(at.col <= line.size());

		if (at.col == line.size()) {
			if (at.line+1 == lines.size()) return;

			line += lines[at.line+1];

			lines.erase(lines.begin() + at.line+1);
		} else {
			line.erase(at.col, 1);
		}
	}

	size_t line_num_width() {
		size_t width = 1;
		size_t lines = this->lines.size();
		while (lines) {
			lines /= 10;
			width += 1;
		}
		return width;
	}
	size_t line_content_width() {
		return vga::WIDTH - line_num_width();
	}
	size_t visual_lines(size_t line_idx) {
		assert(line_idx < lines.size());
		const auto &line = lines[line_idx];
		if (line.size() == 0) return 1;
		const auto content_width = line_content_width();
		return line.size()/content_width + !!(line.size()%content_width);
	}

	// returns { row, col } AKA { y, x }
	Maybe<Pair<size_t, size_t>> find_screen_pos(Pos pos) {
		if (pos.line < screen_top.line) return {};
		if (pos.line == screen_top.line && pos.col < screen_top.col) return {};

		if (pos.line >= screen_top.line + vga::HEIGHT) return {};

		const size_t content_width = line_content_width();

		size_t row = 0;

		Pos at = screen_top;
		while (at.line < pos.line || (at.line == pos.line && at.col + content_width <= pos.col)) {
			advance(at);
			++row;
		}

		const size_t col = at.line > pos.line
			? line_num_width()
			: line_num_width() + pos.col - at.col;

		return { {
			row,
			col
		} };
	}

	void advance(Pos &pos) {
		if (pos.line >= lines.size()) {
			pos.line = lines.size();
			pos.col = 0;
			return;
		}
		if (pos.col >= lines.size()) {
			++pos.line;
			pos.col = 0;
			return;
		}

		const size_t rest = lines[pos.line].size() - pos.col;
		const size_t content_width = line_content_width();

		if (rest > content_width) {
			pos.col += content_width;
		} else {
			++pos.line;
			pos.col = 0;
		}
	}
	void write_from(size_t row, Pos pos) {
		assert(pos.line < lines.size());
		assert(pos.col < lines[pos.line].size() || pos.col == 0);

		const char *const data = lines[pos.line].c_str() + pos.col;
		const size_t data_len = lines[pos.line].size() - pos.col;
		const size_t content_width = line_content_width();

		term::go_to(line_num_width(), row);
		if (data_len == 0) {
			assert(pos.col == 0);
		} else if (data_len > content_width) {
			term::write(data, content_width);
		} else {
			term::writestring(data);
		}
	}
	void write_lineno(size_t row, size_t lineno) {
		const auto _ = sdk::ColorSwitch(vga::Color::Black, vga::Color::LightGrey);

		size_t col = line_num_width() - 2;

		do {
			term::go_to(col, row);
			term::putchar('0' + lineno%10);
			lineno /= 10;
			if (col > 0) --col;
			else return;
		} while (lineno);
		term::go_to(0, row);
		for (size_t c = 0; c <= col; ++c) {
			term::putchar(' ');
		}
	}
	void draw() {
		{
			auto _ = term::Backbuffer();

			term::clear();
			term::go_to(0, 0);

			Pos draw_from = screen_top;
			for (size_t i = 0; i < vga::HEIGHT; ++i) {
				if (draw_from.line >= lines.size()) break;
				if (draw_from.col == 0) {
					write_lineno(i, draw_from.line+1);
				}
				write_from(i, draw_from);
				advance(draw_from);
			}
		}

		const auto cursor_pos = find_screen_pos(cursor);
		assert(cursor_pos.has);
		term::cursor::go_to(cursor_pos.get().second, cursor_pos.get().first);
	}
};

File file {};

State state;

void input_key(ps2::Key key, char ch, bool capitalise) {
	if (key == ps2::KEY_BACKSPACE) {
		if (file.cursor.line == 0 && file.cursor.col == 0) {
			return;
		}

		if (file.cursor.col) {
			--file.cursor.col;
		} else {
			--file.cursor.line;
			file.cursor.col = file.lines[file.cursor.line].size();
		}

		file.delete_character(file.cursor);
	} else if (key == ps2::KEY_ENTER) {
		file.insert_newline(file.cursor);

		++file.cursor.line;
		file.cursor.col = 0;
	} else {
		if (capitalise && 'a' <= ch && ch <= 'z') ch ^= 'a'^'A';

		file.insert_character(file.cursor, ch);

		++file.cursor.col;
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

	term::disable_autoscroll();

	file.draw();

	while (!state.should_quit) {
		const bool had_events = !ps2::events.empty();

		while (!ps2::events.empty()) {
			const auto event = ps2::events.pop();
			handle_keyevent(event.type, event.key);
		}

		if (had_events) file.draw();

		__asm__ volatile("hlt" ::: "memory");
	}
}

}
