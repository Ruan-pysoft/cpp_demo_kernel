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

#define error(msg) do { \
		state.error = msg; \
		state.error_handled = false; \
		return; \
	} while (0)
#define error_fun(fun, msg) error("Error in `" fun "`: " msg)
#define check_stack_len_lt(fun, expr) if (length(state.stack) >= (expr)) error_fun(fun, "stack length should be < " #expr)
#define check_stack_len_ge(fun, expr) if (length(state.stack) < (expr)) error_fun(fun, "stack length should be >= " #expr)
#define check_stack_cap(fun, expr) if (length(state.stack) + (expr) >= STACK_SIZE) error_fun(fun, "stack capacity should be at least " #expr)
#define check_code_len(fun, len) if (length(state.code) + (len) > CODE_BUFFER_SIZE) error_fun(fun, "not enough space to generate code for user word")
using pstate_t = ProgramState;
Primitive primitives[] = {
	/* STACK OPERATIONS */
	{ ".", "-- ; shows the top 16 elements of the stack", [](pstate_t &state) {
		if (length(state.stack) == 0) { puts("empty."); return; }

		const size_t amt = length(state.stack) < 16
			? length(state.stack)
			: 16;
		if (length(state.stack) > 16) {
			term::writestring("... ");
		}
		size_t i = amt;
		while (i --> 0) {
			printf("%d ", stack_peek(state.stack, i));
		}
		putchar('\n');
	} },
	{ "stack_len", "-- a ; pushes length of stack", [](pstate_t &state) {
		check_stack_cap("stack_len", 1);
		push(state.stack, {
			.pos = length(state.stack)
		});
	} },
	{ "dup", "a -- a a", [](pstate_t &state) {
		check_stack_len_ge("dup", 1);
		check_stack_cap("dup", 1);
		push(state.stack, stack_peek(state.stack));
	} },
	{ "swap", "a b -- b a", [](pstate_t &state) {
		check_stack_len_ge("swap", 2);
		const number_t top = pop(state.stack);
		const number_t under_top = pop(state.stack);
		push(state.stack, top);
		push(state.stack, under_top);
	} },
	{ "rot", "a b c -- b c a", [](pstate_t &state) {
		check_stack_len_ge("rot", 3);
		const number_t c = pop(state.stack);
		const number_t b = pop(state.stack);
		const number_t a = pop(state.stack);
		push(state.stack, b);
		push(state.stack, c);
		push(state.stack, a);
	} },
	{ "unrot", "a b c -- c a b", [](pstate_t &state) {
		check_stack_len_ge("rot", 3);
		const number_t c = pop(state.stack);
		const number_t b = pop(state.stack);
		const number_t a = pop(state.stack);
		push(state.stack, c);
		push(state.stack, a);
		push(state.stack, b);
	} },
	{ "rev", "a b c -- c b a", [](pstate_t &state) {
		check_stack_len_ge("rev", 3);
		const number_t c = pop(state.stack);
		const number_t b = pop(state.stack);
		const number_t a = pop(state.stack);
		push(state.stack, c);
		push(state.stack, b);
		push(state.stack, a);
	} },
	{ "drop", "a --", [](pstate_t &state) {
		check_stack_len_ge("drop", 1);
		pop(state.stack);
	} },
	{ "rev_n", "... n -- ... ; reverse the top n elements", [](pstate_t &state) {
		check_stack_len_ge("rev_n", 1);
		const uint32_t n = pop(state.stack).pos;
		check_stack_len_ge("rot_n", n);
		for (size_t i = 0; i < n/2; ++i) {
			const size_t fst_ix = i;
			const size_t scd_ix = n-i-1;
			const number_t tmp = stack_peek(state.stack, fst_ix);
			stack_peek(state.stack, fst_ix) = stack_peek(state.stack, scd_ix);
			stack_peek(state.stack, scd_ix) = tmp;
		}
	} },
	{ "nth", "... n -- ... x ; dup the nth element down to the top", [](pstate_t &state) {
		check_stack_len_ge("nth", 1);
		const uint32_t n = pop(state.stack).pos;
		check_stack_len_ge("nth", n);
		if (n == 0) {
			error_fun("nth", "n must be nonzero");
		}
		push(state.stack, stack_peek(state.stack, n-1));
	} },

	/* ARYTHMETIC OPERATIONS */
	{ "inc", "a -- a+1", [](pstate_t &state) {
		check_stack_len_ge("inc", 1);
		++stack_peek(state.stack).pos;
	} },
	{ "dec", "a -- a-1", [](pstate_t &state) {
		check_stack_len_ge("dec", 1);
		--stack_peek(state.stack).pos;
	} },
	{ "+", "a b -- a+b", [](pstate_t &state) {
		check_stack_len_ge("+", 2);
		push(state.stack, {
			.pos = pop(state.stack).pos + pop(state.stack).pos,
		});
	} },
	{ "*", "a b -- a*b", [](pstate_t &state) {
		check_stack_len_ge("*", 2);
		push(state.stack, {
			.sign = pop(state.stack).sign * pop(state.stack).sign,
		});
	} },
	{ "/", "a b -- a/b", [](pstate_t &state) {
		check_stack_len_ge("/", 2);
		const number_t b = pop(state.stack);
		const number_t a = pop(state.stack);
		push(state.stack, {
			.sign = a.sign / b.sign,
		});
	} },

	/* BITWISE OPERATIONS */
	{ "shl", "a b -- a<<b", [](pstate_t &state) {
		check_stack_len_ge("shl", 2);
		const uint32_t top = pop(state.stack).pos;
		const uint32_t under_top = pop(state.stack).pos;
		if (top >= 32) {
			push(state.stack, {0});
		} else {
			push(state.stack, { .pos = under_top << top });
		}
	} },
	{ "shr", "a b -- a>>b", [](pstate_t &state) {
		check_stack_len_ge("shr", 2);
		const uint32_t top = pop(state.stack).pos;
		const uint32_t under_top = pop(state.stack).pos;
		if (top >= 32) {
			push(state.stack, {0});
		} else {
			push(state.stack, { .pos = under_top >> top });
		}
	} },
	{ "or", "a b -- a|b", [](pstate_t &state) {
		check_stack_len_ge("or", 2);
		push(state.stack, {
			.pos = pop(state.stack).pos | pop(state.stack).pos
		});
	} },
	{ "and", "a b -- a&b", [](pstate_t &state) {
		check_stack_len_ge("and", 2);
		push(state.stack, {
			.pos = pop(state.stack).pos & pop(state.stack).pos
		});
	} },
	{ "xor", "a b -- a^b", [](pstate_t &state) {
		check_stack_len_ge("xor", 2);
		push(state.stack, {
			.pos = pop(state.stack).pos ^ pop(state.stack).pos
		});
	} },
	{ "not", "a -- ~a", [](pstate_t &state) {
		check_stack_len_ge("not", 1);
		push(state.stack, { .pos = ~pop(state.stack).pos });
	} },

	/* COMPARISON */
	{ "=", "a b -- a=b", [](pstate_t &state) {
		check_stack_len_ge("=?", 2);
		push(state.stack, {
			.sign = pop(state.stack).pos == pop(state.stack).pos
			? -1 : 0
		});
	} },
	{ "<", "a b -- a<b", [](pstate_t &state) {
		check_stack_len_ge("=?", 2);
		const int32_t b = pop(state.stack).sign;
		const int32_t a = pop(state.stack).sign;
		push(state.stack, { .sign = a < b ? -1 : 0 });
	} },

	/* LITERALS */
	{ "true", "-- -1", [](pstate_t &state) {
		check_stack_len_lt("true", STACK_SIZE);
		push(state.stack, { .sign = -1 });
	} },
	{ "false", "-- 0", [](pstate_t &state) {
		check_stack_len_lt("false", STACK_SIZE);
		push(state.stack, { .sign = 0 });
	} },

	/* OUTPUT OPERATIONS */
	{ "print", "a -- ; prints top element of stack as a signed number", [](pstate_t &state) {
		check_stack_len_ge("print", 1);
		printf("%d ", pop(state.stack).sign);
	} },
	{ "pstr", "a -- ; prints top element as string of at most four characters", [](pstate_t &state) {
		check_stack_len_ge("pstr", 1);
		const uint32_t str_raw = pop(state.stack).pos;
		const char *str = (char*)&str_raw;
		for (size_t i = 0; i < 4; ++i) {
			if (str[i] == 0) break;
			term::putchar(str[i]);
		}
	} },

	/* STRINGS */
	{ "print_string", "... n -- ; prints a string of length n", [](pstate_t &state) {
		check_stack_len_ge("print_string", 1);
		const uint32_t n = pop(state.stack).pos;

		check_stack_len_ge("print_string", n);
		term::writestring((const char*)&stack_peek(state.stack, n-1));
		for (uint32_t i = 0; i < n; ++i) pop(state.stack);
	} },

	/* SYSTEM OPERATION */
	{ "exit", "-- ; exits the mieliepit interpreter", [](pstate_t&) {
		state_ptr->should_quit = true;
	} },
	{ "quit", "-- ; exits the mieliepit interpreter", [](pstate_t&) {
		state_ptr->should_quit = true;
	} },
	// TODO: sleep functions perhaps, clearing the keyboard buffer when done? Essentially ignoring all user input while sleeping

	/* DOCUMENTATION / HELP / INSPECTION */
	{ "syntax", "-- ; prints a list of all available syntax items", [](pstate_t &state) {
		for (idx_t i = 0; i < syntax_len; ++i) {
			if (i) putchar(' ');
			term::writestring(state.syntax[i].name);
		}
		putchar('\n');
	} },
	{ "primitives", "-- ; prints a list of all available primitive words", [](pstate_t &state) {
		for (idx_t i = 0; i < state.primitives_len; ++i) {
			if (i) putchar(' ');
			term::writestring(state.primitives[i].name);
		}
		putchar('\n');
	} },
	{ "words", "-- ; prints a list of all user-defined words", [](pstate_t &state) {
		size_t i = length(state.words);
		while (i --> 0) {
			term::writestring(state.words[i].name);
			if (i) putchar(' ');
		}
		putchar('\n');
	} },
	{ "guide", "-- ; prints usage guide for the mieliepit interpreter", [](pstate_t&) {
		const char *guide_text =
			"Mieliepit is a stack-based programming language.\n"
			"It is operated by entering a sequence of space-seperated words into the prompt.\n"
			"Data consists of 32-bit integers stored on a stack. You can enter a single period ( . ) into the prompt at any time to view the stack.\n"
			"Comments are formed with parentheses: ( this is a comment ) . Remember to leave spaces around each parenthesis!\n"
			"There are two kinds of words: Primitives, which perform some operation, and numbers, which pushes a number to the stack.\n"
			"To get a list of available primitives, enter `primitives` into the prompt.\n"
			"To get more information on a given primitive, enter `help` followed by its name. Try `help help` or `help guide`.\n"
			"A simple hello world program is `' hell pstr ' o pstr 32 pstr ' worl pstr ' d! pstr`. See if you can figure out how it works.\n"
			"To get a list of available words, enter `words` into the prompt.\n"
			"To see what a word was compiled into, enter `def` followed by its name. Try `def neg`.\n"
			"To define your own word, start with `:`, followed by its name, then some documentation in a comment, then its code, ending off with `;`.\n"
			"An example word definition would be : test ( this is an example ) ' test pstr ; . See if you can define your own plus function using `-` and `neg`.\n"
		;
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
	} },
};

#undef error_fun
#undef error

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
		.action = Interpreter::Run,
		.curr_word = {},
		.state = state_ptr->program_state,
	};
	state_ptr->program_state.error = nullptr;
	state_ptr->program_state.error_handled = false;

	while (!state_ptr->program_state.error && interpreter.len > 0) {
		interpreter.advance();
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
			|| key_state[KEY_RSHIFT]
			|| state_ptr->capslock;
		input_key(key, key_ascii_map[key], capitalise);
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
		term::cursor::enable(8, 15);
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
		term::cursor::enable(8, 15);
		return;
	}
}

void interpret_str(const char *str) {
	Interpreter interpreter {
		.line = str,
		.len = strlen(str),
		.action = Interpreter::Run,
		.curr_word = {},
		.state = state_ptr->program_state,
	};
	state_ptr->program_state.error = nullptr;
	state_ptr->program_state.error_handled = false;

	while (!state_ptr->program_state.error && interpreter.len > 0) {
		interpreter.advance();
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

void main() {
	assert(!mieliepit_running);
	mieliepit_running = true;
	State state{
		.program_state = ProgramState(
			primitives, sizeof(primitives)/sizeof(*primitives),
			syntax, syntax_len
		),
	};
	state_ptr = &state;

	interpret_str(": - ( a b -- a-b ) not inc + ;");
	interpret_str(": neg ( a -- -a ) 0 swap - ;");

	interpret_str(": *_under ( a b -- a a*b ) swap dup rot * ;");
	// TODO: the following doesn't compile, since rep isn't defined yet
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
