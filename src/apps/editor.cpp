#include "apps/editor.hpp"

#include <stdlib.h>

#include <sdk/util.hpp>
#include <sdk/terminal.hpp>

#include "ps2.hpp"
#include "vga.hpp"

#include "apps/components/menu.hpp"

namespace editor {

namespace {

using namespace sdk::util;

struct State;
extern State state;

struct File {
	struct Pos {
		size_t line = 0;
		size_t col = 0;

		bool operator==(const Pos &other) {
			return line == other.line && col == other.col;
		}
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
		if (pos.col >= lines[pos.line].size()) {
			if (pos.line != lines[pos.line].size()-1) {
				++pos.line;
				pos.col = 0;
			} else pos.col = lines[pos.line].size();
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

	Pos advance_line_forwards(Pos pos) {
		const size_t content_width = line_content_width();

		if (lines.size() == 0) {
			return { .line = 0, .col = 0 };
		} else if (pos.line >= lines.size()) {
			Pos res = {
				.line = lines.size()-1,
				.col = 0,
			};

			while (res.col + content_width < lines.back().size()) {
				res.col += content_width;
			}

			return res;
		} else if (pos.col >= lines[pos.line].size()) {
			if (pos.line != lines[pos.line].size()-1) {
				return {
					.line = pos.line+1,
					.col = 0,
				};
			} else {
				Pos res = {
					.line = pos.line,
					.col = 0,
				};

				while (res.col + content_width < lines[res.line].size()) {
					res.col += content_width;
				}

				return res;
			}
		}

		if (pos.col + content_width < lines[pos.line].size()) {
			return {
				.line = pos.line,
				.col = pos.col + content_width,
			};
		} else if (pos.line + 1 < lines.size()) {
			return {
				.line = pos.line+1,
				.col = 0,
			};
		} else return pos;
	}
	Pos advance_line_backwards(Pos pos) {
		const size_t content_width = line_content_width();

		if (lines.size() == 0) {
			return { .line = 0, .col = 0 };
		} else if (pos.line >= lines.size()) {
			Pos res = {
				.line = lines.size()-1,
				.col = 0,
			};

			while (res.col + content_width < lines.back().size()) {
				res.col += content_width;
			}

			return res;
		} else if (pos.line == 0 && pos.col == 0) {
			return pos;
		}

		if (pos.col == 0) {
			Pos res = {
				.line = pos.line-1,
				.col = 0,
			};

			while (res.col + content_width < lines[res.line].size()) {
				res.col += content_width;
			}

			return res;
		} else if (pos.col <= content_width) {
			return {
				.line = pos.line,
				.col = 0,
			};
		} else {
			return {
				.line = pos.line,
				.col = pos.col - content_width,
			};
		}
	}

	void move_left() {
		if (cursor.col) --cursor.col;
		else if (cursor.line) {
			--cursor.line;
			cursor.col = lines[cursor.line].size();
		}

		scroll();
	}
	void move_right() {
		if (cursor.col == lines[cursor.line].size()) {
			if (cursor.line + 1 < lines.size()) {
				++cursor.line;
				cursor.col = 0;
			}
		} else {
			++cursor.col;
		}

		scroll();
	}
	void move_up() {
		if (cursor.line) {
			--cursor.line;
			if (cursor.col > lines[cursor.line].size()) {
				cursor.col = lines[cursor.line].size();
			}
		}

		scroll();
	}
	void move_down() {
		if (cursor.line + 1 < lines.size()) {
			++cursor.line;
			if (cursor.col > lines[cursor.line].size()) {
				cursor.col = lines[cursor.line].size();
			}
		}

		scroll();
	}

	void scroll() {
		const auto screen_pos = find_screen_pos(cursor);

		if (!screen_pos.has) {
			screen_top = cursor;

			screen_top = advance_line_backwards(screen_top);
			screen_top = advance_line_backwards(screen_top);
		} else {
			size_t y = screen_pos.get().first;

			while (y < 2) {
				screen_top = advance_line_backwards(screen_top);
				++y;
			}
			while (y+1 >= vga::HEIGHT - 2) {
				screen_top = advance_line_forwards(screen_top);
				--y;
			}
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
	void write_lineno(size_t row, size_t lineno, bool highlight = false) {
		const auto _ = highlight
			? sdk::ColorSwitch(vga::Color::LightBrown, vga::Color::DarkGrey)
			: sdk::ColorSwitch(vga::Color::LightGrey, vga::Color::DarkGrey)
		;

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
	void draw();
};

struct State {
	enum class View {
		MainMenu,
		OpenMenu,
		DeleteMenu,
		FileEdit,
	};

	View curr_view = View::MainMenu;

	bool should_quit = false;
	bool capslock = false;
	bool relative_line_numbers = false;

	Maybe<File*> curr_file {};
};

State state;

// Keep files as a global static variable
// so that it doesn't get reset on program start
List<File> files {};

List<menu::Entry<State &>> main_menu_items({
	{ "Open File", [](State &state) {
		state.curr_view = State::View::OpenMenu;
	}, state },
	{ "Delete File", [](State &state) {
		state.curr_view = State::View::DeleteMenu;
	}, state },
	{ "New File", [](State &state) {
		files.push_back({});
		state.curr_file.set(&files.back());
		state.curr_view = State::View::FileEdit;
	}, state },
	{ "Quit", [](State &state) {
		state.should_quit = true;
	}, state },
});

menu::Menu<State &> main_menu {
	main_menu_items, {},
	"Editor -- Main Menu"
};

void File::draw() {
	{
		auto _ = term::Backbuffer();

		term::clear();
		term::go_to(0, 0);

		Pos draw_from = screen_top;
		for (size_t i = 0; i < vga::HEIGHT; ++i) {
			if (draw_from.line >= lines.size()) break;
			if (draw_from.col == 0) {
				if (state.relative_line_numbers && draw_from.line != cursor.line) {
					const size_t rel = draw_from.line < cursor.line
						? cursor.line - draw_from.line
						: draw_from.line - cursor.line
					;
					write_lineno(i, rel);
				} else if (state.relative_line_numbers) {
					write_lineno(i, draw_from.line+1, true);
				} else {
					write_lineno(i, draw_from.line+1);
				}
			}
			write_from(i, draw_from);
			advance(draw_from);
		}
	}

	const auto cursor_pos = find_screen_pos(cursor);
	assert(cursor_pos.has);
	term::cursor::go_to(cursor_pos.get().second, cursor_pos.get().first);
}

void input_key(File &file, ps2::Key key, char ch, bool capitalise) {
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
	} else if (key == ps2::KEY_DELETE) {
		if (file.cursor.line == file.lines.size() && file.cursor.col == file.lines.back().size()) {
			return;
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

	file.scroll();
}

void handle_keyevent(ps2::EventType type, ps2::Key key) {
	using namespace ps2;

	if (state.curr_view == State::View::FileEdit) {
		assert(state.curr_file.has);

		File &file = *state.curr_file.get();

		if (type != EventType::Press && type != EventType::Bounce) return;

		const bool command = key_state[KEY_LCTL] || key_state[KEY_RCTL];

		if (!command && (key_ascii_map[key] || key == KEY_BACKSPACE || key == KEY_DELETE)) {
			const bool capitalise = key_state[KEY_LSHIFT]
				|| key_state[KEY_RSHIFT];
			input_key(file, key, key_ascii_map[key], capitalise ^ state.capslock);
			return;
		}

		if (key == KEY_CAPSLOCK) {
			state.capslock = !state.capslock;
		}

		if (key == KEY_ESCAPE) {
			state.should_quit = true;
		}

		if (command && key == KEY_R) {
			state.relative_line_numbers = !state.relative_line_numbers;
		}

		// TODO: word navigation when holding in control?
		if (key == KEY_LEFT) {
			file.move_left();
		} else if (key == KEY_RIGHT) {
			file.move_right();
		} else if (key == KEY_UP) {
			file.move_up();
		} else if (key == KEY_DOWN) {
			file.move_down();
		}
	} else if (state.curr_view == State::View::MainMenu) {
		main_menu.handle_key({ .key = key, .type = type });
	} else assert(false);
}

}

void main() {
	state = State{};

	term::disable_autoscroll();

	main_menu.draw();

	while (!state.should_quit) {
		const bool had_events = !ps2::events.empty();

		while (!ps2::events.empty()) {
			const auto event = ps2::events.pop();
			handle_keyevent(event.type, event.key);
		}

		if (had_events) switch (state.curr_view) {
			case State::View::MainMenu: {
				main_menu.draw();
			} break;
			case State::View::OpenMenu: {
				assert(false);
			} break;
			case State::View::DeleteMenu: {
				assert(false);
			} break;
			case State::View::FileEdit: {
				assert(state.curr_file.has);

				state.curr_file.get()->draw();
			} break;
		}

		__asm__ volatile("hlt" ::: "memory");
	}
}

}
