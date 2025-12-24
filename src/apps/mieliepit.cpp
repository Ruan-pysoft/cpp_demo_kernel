#include "apps/mieliepit.hpp"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <sdk/terminal.hpp>
#include <sdk/util.hpp>

#include "mieliepit/mieliepit.hpp"

#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace mieliepit {

namespace {

constexpr size_t MAX_LINE_LEN = vga::WIDTH - 3;
constexpr size_t LINE_BUF_LEN = 128; // in order to support more complex pre-defined words
static_assert(LINE_BUF_LEN >= MAX_LINE_LEN, "a line should be able to contain at least MAX_LINE_LEN characters");
struct State {
	bool should_quit = false;
	bool capslock = false;
	size_t line_len = 0;
	char line[LINE_BUF_LEN] {};
	ProgramState program_state;
	bool has_inp_err = false;
	uint32_t inp_err_until = 0;

	State(const State&) = delete;
	State &operator=(const State&) = delete;
	State(State&&) = default;
	State &operator=(State&&) = default;
};
static State *state_ptr = nullptr;
bool mieliepit_running = false;

using namespace sdk::util;

void input_key(ps2::Key, char ch, bool capitalise) {
	if (state_ptr->line_len == MAX_LINE_LEN) {
		state_ptr->has_inp_err = true;
		state_ptr->inp_err_until = pit::millis + 100;

		const auto _ = sdk::ColorSwitch(
			vga::Color::Black,
			vga::Color::LightRed
		);

		term::advance();
		term::backspace();
		return;
	}
	// TODO: capitalisation for accented letters?
	// wait, I can't enter those on the keyboard anyways...
	// first I'd need a way to enter them
	if (capitalise && 'a' <= ch && ch <= 'z') ch ^= 'a'^'A';
	state_ptr->line[state_ptr->line_len++] = ch;
	putchar(ch);

	if (state_ptr->line_len == MAX_LINE_LEN) term::cursor::disable();
}

void interpret_line() {
	Interpreter interpreter {
		.line = state_ptr->line,
		.len = state_ptr->line_len,
		.curr_word = {},
		.state = state_ptr->program_state,
	};
	state_ptr->program_state.error = nullptr;
	state_ptr->program_state.error_handled = false;

	while (!state_ptr->program_state.error && interpreter.len > 0) {
		interpreter.run_next();
	}

	if (state_ptr->program_state.error) {
		const auto _ = sdk::ColorSwitch(vga::Color::LightRed);

		if (!state_ptr->program_state.error_handled) {
			putchar('\n');
			puts(state_ptr->program_state.error);
		}
		if (interpreter.len == 0) {
			puts("@ end of line");
		} else {
			printf(
				"@ word starting at %u: ",
				interpreter.line - state_ptr->line
			);
			for (size_t i = 0; i < interpreter.curr_word.len; ++i) {
				putchar(interpreter.curr_word.text[i]);
			}
			putchar('\n');
		}

		state_ptr->program_state.error_handled = true;
	} else {
		putchar('\n');
	}
}

void handle_keyevent(ps2::EventType type, ps2::Key key) {
	using namespace ps2;

	if (type != EventType::Press && type != EventType::Bounce) return;

	if (key_ascii_map[key] && key != KEY_ENTER) {
		const bool capitalise = key_state[KEY_LSHIFT]
			|| key_state[KEY_RSHIFT];
		input_key(key, key_ascii_map[key], capitalise ^ state_ptr->capslock);
		return;
	}

	if (key == KEY_BACKSPACE) {
		if (state_ptr->line_len == 0) return;

		if (state_ptr->has_inp_err) {
			state_ptr->has_inp_err = false;

			term::advance();
			term::backspace();
		}

		--state_ptr->line_len;
		term::backspace();
		term::cursor::enable();
	}

	if (key == KEY_ENTER) {
		if (state_ptr->has_inp_err) {
			state_ptr->has_inp_err = false;

			term::advance();
			term::backspace();
		}

		putchar('\n');
		interpret_line();
		state_ptr->line_len = 0;
		term::writestring("> ");
		term::cursor::enable();
		return;
	}

	if (key == KEY_CAPSLOCK) {
		state_ptr->capslock = !state_ptr->capslock;
	}
}

void interpret_str(const char *str) {
	Interpreter interpreter {
		.line = str,
		.len = strlen(str),
		.curr_word = {},
		.state = state_ptr->program_state,
	};
	state_ptr->program_state.error = nullptr;
	state_ptr->program_state.error_handled = false;

	while (!state_ptr->program_state.error && interpreter.len > 0) {
		interpreter.run_next();
	}

	if (state_ptr->program_state.error) {
		const auto _ = sdk::ColorSwitch(vga::Color::LightRed);

		if (!state_ptr->program_state.error_handled) {
			putchar('\n');
			puts(state_ptr->program_state.error);
		}
		if (interpreter.len == 0) {
			puts("@ end of line");
		} else {
			printf(
				"@ word starting at %u: ",
				interpreter.line - str
			);
			for (size_t i = 0; i < interpreter.curr_word.len; ++i) {
				putchar(interpreter.curr_word.text[i]);
			}
			putchar('\n');
		}

		state_ptr->program_state.error_handled = true;
	} else {
		putchar('\n');
	}
}

}

void quit_primitive_fn(ProgramState &) {
	state_ptr->should_quit = true;
}

void guide_primitive_fn(ProgramState &) {
	size_t guide_len = strlen(guide_text);
	size_t pos = 0;
	size_t line_len = 0;
	size_t word_start = 0;
	while (pos < guide_len) {
		const char ch = guide_text[pos];
		if (ch == ' ' && line_len == vga::WIDTH) {
			line_len = 0;
			while (pos < guide_len && guide_text[pos] == ' ') ++pos;
		} else if (ch == ' ' && line_len == vga::WIDTH-1) {
			putchar('\n');
			line_len = 0;
			while (pos < guide_len && guide_text[pos] == ' ') ++pos;
		} else if (ch == ' ') {
			putchar(ch);
			++line_len;
			++pos;
		} else if (ch == '\n') {
			putchar(ch);
			line_len = 0;
			++pos;
		} else {
			word_start = pos;
			while (pos < guide_len && guide_text[pos] != ' ' && guide_text[pos] != '\n') {
				++pos;
				++line_len;
			}
			if (line_len >= vga::WIDTH) {
				// yes, the >= is intentional, I don't want words bumping up to the edge of the screen exactly.
				putchar('\n');
				line_len = pos - word_start;
			}
			for (size_t i = word_start; i < pos; ++i) putchar(guide_text[i]);
		}
	}
}

void main() {
	assert(!mieliepit_running);
	mieliepit_running = true;
	State state{
		.program_state = ProgramState(
			primitives, PW_COUNT,
			syntax, SC_COUNT
		),
	};
	state_ptr = &state;

	interpret_str(": - ( a b -- a-b ) not inc + ;");
	interpret_str(": neg ( a -- -a ) 0 swap - ;");

	interpret_str(": *_under ( a b -- a a*b ) swap dup rot * ;");
	interpret_str(": ^ ( a b -- a^b ; a to the power b ) 1 swap rep *_under swap drop ;");

	interpret_str(": != ( a b -- a!=b ) = not ;");
	interpret_str(": <= ( a b -- a<=b ) dup rot dup rot < unrot = or ;");
	interpret_str(": >= ( a b -- a>=b ) < not ;");
	interpret_str(": > ( a b -- a>=b ) <= not ;");

	interpret_str(": truthy? ( a -- a!=false ) false != ;");

	interpret_str(": show_top ( a -- a ; prints the topmost stack element ) dup print ;");
	interpret_str(": clear ( ... - ; clears the stack ) stack_len 0 = ? ret drop rec ;");

	term::clear();
	term::go_to(0, 0);
	puts("Enter `guide` for instructions on usage, or `exit` to exit the program.");
	term::writestring("> ");

	/*
	run_line("\" Tests for rep_and: \" print_string");
	run_line("1 0 rep ? help rep . clear");
	run_line("0 1 rep ? help rep . clear");
	run_line(": \255 rep ? help rep ; def \255");
	run_line("1 0 \255 . clear");
	run_line("0 1 \255 . clear");
	*/

	while (!state.should_quit) {
		while (!ps2::events.empty()) {
			const auto event = ps2::events.pop();
			handle_keyevent(event.type, event.key);
		}

		if (state.has_inp_err && pit::millis >= state.inp_err_until) {
			state.has_inp_err = false;

			term::advance();
			term::backspace();
		}

		__asm__ volatile("hlt" ::: "memory");
	}

	mieliepit_running = false;
}

}
