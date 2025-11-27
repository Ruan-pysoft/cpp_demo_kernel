#include "apps/forth.hpp"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <sdk/terminal.hpp>

#include "pit.hpp"
#include "ps2.hpp"
#include "vga.hpp"

namespace forth {

namespace {

struct WordPos {
	size_t pos = 0;
	size_t idx = 0;
	size_t len = 0;
};
struct CodePos {
	size_t pos = 0;
	size_t idx = 0;
	size_t len = 0;
};
struct InterpreterState {
	size_t line_pos = 0;
	WordPos word;
	bool in_code = false;
	CodePos code;
	const char *err = NULL;
	bool err_handled = false;
};

struct PrimitiveEntry {
	const char *name;
	const char *desc;
	void(*func)();
	bool immediate = false;
};
struct Word {
	const char *name;
	const char *desc;
	uint32_t code_pos;
	size_t code_len;
};

struct CompilerState {
	size_t code_start_idx;
	WordPos name;
	size_t desc_start;
	size_t desc_len;
};

enum class CodeElemPrefix {
	Literal = -1,
	Word = -2,
};
static_assert(sizeof(CodeElemPrefix) == sizeof(uint32_t), "Expected enum class to be 32 bits");
union CodeElem {
	CodeElemPrefix prefix;
	uint32_t lit;
	uint32_t idx;
};
static_assert(sizeof(CodeElem) == sizeof(uint32_t), "Expected union of 32-bit values to be 32 bits");

constexpr size_t MAX_LINE_LEN = vga::WIDTH - 3;
constexpr size_t STACK_SIZE = 1024;
constexpr size_t MAX_WORDS = 1024;
constexpr size_t EST_DESC_LEN = 64;
constexpr size_t DESC_BUF_LEN = MAX_WORDS * EST_DESC_LEN;
constexpr size_t EST_NAME_LEN = 4;
constexpr size_t NAME_BUF_LEN = MAX_WORDS * EST_NAME_LEN;
constexpr size_t CODE_BUF_SIZE = MAX_WORDS*64;
struct State {
	bool should_quit = false;
	bool capslock = false;
	size_t line_len = 0;
	char line[MAX_LINE_LEN];
	size_t stack_len = 0;
	uint32_t stack[STACK_SIZE];
	size_t words_len = 0;
	Word words[MAX_WORDS];
	size_t word_descs_len = 0;
	char word_descs[DESC_BUF_LEN];
	size_t word_names_len = 0;
	char word_names[NAME_BUF_LEN];
	size_t code_len;
	CodeElem code[CODE_BUF_SIZE];
	bool has_inp_err = false;
	uint32_t inp_err_until = 0;
	bool compiling = false;
	InterpreterState interp;
	CompilerState comp;

	State(const State&) = delete;
	State &operator=(const State&) = delete;
	State(State&&) = default;
	State &operator=(State&&) = default;
};
static State state{};
bool forth_running = false;

static inline void stack_push(uint32_t val) {
	// WARN: I assume here that there is space on the stack, without verifying the fact!
	state.stack[state.stack_len++] = val;
}
static inline uint32_t stack_pop() {
	// WARN: I assume here that the stack is non-empty, without verifying the fact!
	return state.stack[--state.stack_len];
}
static inline uint32_t stack_peek(size_t nth = 1) {
	// WARN: I assume here that the stack's length is >= nth, without verifying the fact!
	return state.stack[state.stack_len - nth];
}
static inline uint32_t &stack_get(size_t nth = 1) {
	// WARN: I assume here that the stack's length is >= nth, without verifying the fact!
	return state.stack[state.stack_len - nth];
}

void get_word();
int32_t search_primitive(const char *name, size_t name_len);
int32_t search_word(const char *name, size_t name_len);
#define error(msg) do { \
		state.interp.err = msg; \
		return; \
	} while (0)
#define error_fun(fun, msg) do { \
		state.interp.err = "Error in `" fun "`: " msg; \
		return; \
	} while (0)
#define check_stack_len_lt(fun, expr) if (state.stack_len >= (expr)) error_fun(fun, "stack length should be < " #expr)
#define check_stack_len_ge(fun, expr) if (state.stack_len < (expr)) error_fun(fun, "stack length should be >= " #expr)
const PrimitiveEntry primitives[] = {
	{ "dup", "a -- a a", []() {
		check_stack_len_ge("dup", 1);
		check_stack_len_lt("dup", STACK_SIZE);
		stack_push(stack_peek());
	} },
	{ "swap", "a b -- b a", []() {
		check_stack_len_ge("swap", 2);
		const uint32_t top = stack_pop();
		const uint32_t under_top = stack_pop();
		stack_push(top);
		stack_push(under_top);
	} },
	{ "rot", "a b c -- b c a", []() {
		check_stack_len_ge("rot", 3);
		const uint32_t c = stack_pop();
		const uint32_t b = stack_pop();
		const uint32_t a = stack_pop();
		stack_push(b);
		stack_push(c);
		stack_push(a);
	} },
	{ "drop", "a --", []() {
		check_stack_len_ge("drop", 1);
		stack_pop();
	} },
	{ "inc", "a -- a+1", []() {
		check_stack_len_ge("inc", 1);
		++stack_get();
	} },
	{ "dec", "a -- a-1", []() {
		check_stack_len_ge("dec", 1);
		--stack_get();
	} },
	{ "shl", "a b -- a<<b", []() {
		check_stack_len_ge("shl", 2);
		const uint32_t top = stack_pop();
		const uint32_t under_top = stack_pop();
		stack_push(under_top << top);
	} },
	{ "shr", "a b -- a>>b", []() {
		check_stack_len_ge("shr", 2);
		const uint32_t top = stack_pop();
		const uint32_t under_top = stack_pop();
		stack_push(under_top >> top);
	} },
	{ "or", "a b -- a|b", []() {
		check_stack_len_ge("or", 2);
		stack_push(stack_pop() | stack_pop());
	} },
	{ "and", "a b -- a&b", []() {
		check_stack_len_ge("and", 2);
		stack_push(stack_pop() & stack_pop());
	} },
	{ "xor", "a b -- a^b", []() {
		check_stack_len_ge("xor", 2);
		stack_push(stack_pop() ^ stack_pop());
	} },
	{ "not", "a -- ~a", []() {
		check_stack_len_ge("not", 1);
		stack_push(~stack_pop());
	} },
	{ "true", "-- -1", []() {
		check_stack_len_lt("true", STACK_SIZE);
		stack_push(~0);
	} },
	{ "false", "-- 0", []() {
		check_stack_len_lt("false", STACK_SIZE);
		stack_push(0);
	} },
	{ "+", "a b -- a+b", []() {
		check_stack_len_ge("+", 2);
		stack_push(stack_pop() + stack_pop());
	} },
	{ "print", "a -- ; prints top element of stack as a signed number", []() {
		check_stack_len_ge("print", 1);
		printf("%d ", reinterpret_cast<uint32_t>(stack_pop()));
	} },
	{ "pstr", "a -- ; prints top element as string of at most four characters", []() {
		check_stack_len_ge("pstr", 1);
		const uint32_t str_raw = stack_pop();
		const char *str = (char*)&str_raw;
		term::writestring(str);
	} },
	{ "print_raw", "a -- ; interprets top elemt of stack as a character pointer and prints it as a c-string", []() {
		check_stack_len_ge("print_raw", 1);
		const char *str = reinterpret_cast<const char*>(stack_pop());
		term::writestring(str);
	} },
	{ "stack_len", "-- a ; pushes length of stack", []() {
		check_stack_len_lt("stack_len", STACK_SIZE);
		stack_push(state.stack_len);
	} },
	{ "exit", "-- ; exits the forth interpreter", []() {
		state.should_quit = true;
	} },
	{ "quit", "-- ; exits the forth interpreter", []() {
		state.should_quit = true;
	} },
	{ "hex", "-- a ; interprets next word as hex number and pushes it", []() {
		check_stack_len_lt("hex", STACK_SIZE);
		get_word();
		if (state.interp.word.len == 0) {
			error_fun("hex", "expected a hexadecimal number, didn't get anything");
		}
		if (state.interp.word.len > 8) {
			error_fun("hex", "largest supported number is FFFFFFFF");
		}
		uint32_t num = 0;
		for (size_t i = 0; i < state.interp.word.len; ++i) {
			const char ch = state.line[state.interp.word.pos+i];
			if ((ch < '0' || '9' < ch) && (ch < 'a' || 'z' < ch) && (ch < 'A' || 'Z' < ch)) {
				error_fun("hex", "expected hex number to consist only of hex digits (0-9, a-f, A-F)");
			}
			num <<= 4;
			if ('0' <= ch && ch <= '9') {
				num |= ch-'0';
			} else if ('a' <= ch && ch <= 'z') {
				num |= ch-'a';
			} else {
				num |= ch-'A';
			}
		}
		if (state.compiling) {
			if (state.code_len + 2 > CODE_BUF_SIZE) {
				error_fun("hex", "not enough space to generate code for user word");
			}
			state.code[state.code_len++] = CodeElem {
				.prefix = CodeElemPrefix::Literal,
			};
			state.code[state.code_len++] = CodeElem {
				.lit = num,
			};
		} else {
			stack_push(num);
		}
	}, true },
	{ "'", "-- a ; interprets next word as short (<= 4 long) string and pushes it", []() {
		check_stack_len_lt("'", STACK_SIZE);
		get_word();
		if (state.interp.word.len == 0) {
			error_fun("'", "expected a short string, didn't get anything");
		}
		if (state.interp.word.len > 8) {
			error_fun("'", "short strings may be no longer than four characters");
		}
		uint32_t num = 0;
		size_t i = state.interp.word.len;
		while (i --> 0) {
			const char ch = state.line[state.interp.word.pos+i];
			num <<= 8;
			num |= *(const uint8_t*)&ch;
		}
		if (state.compiling) {
			if (state.code_len + 2 > CODE_BUF_SIZE) {
				error_fun("'", "not enough space to generate code for user word");
			}
			state.code[state.code_len++] = CodeElem {
				.prefix = CodeElemPrefix::Literal,
			};
			state.code[state.code_len++] = CodeElem {
				.lit = num,
			};
		} else {
			stack_push(num);
		}
	}, true },
	{ "help", "-- ; prints help text for the next word", []() {
		get_word();
		if (state.interp.word.len == 0) {
			error_fun("help", "expected following word");
		}

		const int32_t word_idx = search_word(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		);
		if (word_idx != -1 && state.compiling) {
			if (state.code_len + 3 > CODE_BUF_SIZE) {
				error_fun("help", "not enough space to generate code for user word");
			}

			state.code[state.code_len++] = CodeElem {
				.prefix = CodeElemPrefix::Literal,
			};
			state.code[state.code_len++] = CodeElem {
				.lit = reinterpret_cast<uint32_t>(state.words[word_idx].desc),
			};
			const int32_t print_raw = search_primitive("print_raw", 9);
			state.code[state.code_len++] = CodeElem {
				.idx = *(uint32_t*)&print_raw,
			};

			return;
		} else if (word_idx != -1) {
			printf(
				"`%s`: %s",
				state.words[word_idx].name,
				state.words[word_idx].desc
			);

			return;
		}

		const int32_t prim_idx = search_primitive(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		);
		if (prim_idx != -1 && state.compiling) {
			if (state.code_len + 3 > CODE_BUF_SIZE) {
				error_fun("help", "not enough space to generate code for user word");
			}

			state.code[state.code_len++] = CodeElem {
				.prefix = CodeElemPrefix::Literal,
			};
			state.code[state.code_len++] = CodeElem {
				.lit = reinterpret_cast<uint32_t>(primitives[prim_idx].desc),
			};
			const int32_t print_raw = search_primitive("print_raw", 9);
			state.code[state.code_len++] = CodeElem {
				.idx = *(uint32_t*)&print_raw,
			};

			return;
		} else if (prim_idx == -1) {
			printf(
				"`%s`: %s",
				primitives[prim_idx].name,
				primitives[prim_idx].desc
			);

			return;
		}

		error_fun("help", "Couldn't find specified word");
	}, true },
	{ "primitives", "-- ; prints a list of all available primitive words", []() {
		for (size_t pi = 0; primitives[pi].func != NULL; ++pi) {
			if (pi) putchar(' ');
			term::writestring(primitives[pi].name);
		}
		putchar('\n');
	} },
	{ "words", "-- ; prints a list of all user-defined words", []() {
		size_t wi = state.words_len;
		while (wi --> 0) {
			term::writestring(state.words[wi].name);
			if (wi) putchar(' ');
		}
		putchar('\n');
	} },
	{ "def", "-- ; prints the definition of a given word", []() {
		get_word();
		if (state.interp.word.len == 0) {
			error_fun("def", "expected following word");
		}

		if (state.compiling) {
			// TODO: support this
			// I think I should add a hidden flag + an extra primitive?
			error_fun("def", "printing the definition of a word from within a compiled word is currently not supported");
		}

		const int32_t word_idx = search_word(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		);
		if (word_idx != -1) {
			printf(
				": %s ( %s )",
				state.words[word_idx].name,
				state.words[word_idx].desc
			);
			// no checking as to validity
			for (size_t ci = state.words[word_idx].code_pos; ci < state.words[word_idx].code_pos + state.words[word_idx].code_len; ++ci) {
				const CodeElem head = state.code[ci];
				if (head.prefix == CodeElemPrefix::Literal) {
					++ci;
					printf(" %u", state.code[ci].lit);
				} else if (head.prefix == CodeElemPrefix::Word) {
					++ci;
					printf(" %s", state.words[state.code[ci].lit].name);
				} else {
					printf(" %s", primitives[head.idx]);
				}
			}
			printf(" ;");

			return;
		}

		const int32_t prim_idx = search_primitive(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		);
		if (prim_idx != -1) {
			printf(
				"<built-in primitive `%s`>",
				primitives[prim_idx].name
			);

			return;
		}

		error_fun("def", "Couldn't find specified word");
	}, true },
	{ "guide", "-- ; prints usage guide for the forth interpreter", []() {
		const char *guide_text =
			"This is a FORTH interpreter. It is operated by entering a sequence of space-seperated words into the prompt.\n"
			"Data consists of 32-bit integers stored on a stack.\n"
			"There are two kinds of words: Primitives, which perform some operation, and numbers, which pushes a number to the stack.\n"
			"To get a list of available primitives, enter `primitives` into the prompt.\n"
			"To get more information on a given primitive, enter `help` followed by its name. Try `help help` or `help guide`.\n"
			"A simple hello world program is `' hell pstr ' o pstr 32 pstr ' worl pstr ' d! pstr`. See if you can figure out how it works.\n"
		;
		term::writestring(guide_text);
	} },
	{ "(", "-- ; begins a comment", []() {
		int nesting = 1;

		while (nesting) {
			get_word();
			if (state.interp.word.len == 0) {
				error_fun("(", "expected matching )");
			}

			if (state.interp.word.len == 1 && state.line[state.interp.word.pos] == '(') {
				++nesting;
			} else if (state.interp.word.len == 1 && state.line[state.interp.word.pos] == ')') {
				--nesting;
			}
		}
	}, true },
	{ ")", "-- ; ends a comment", []() {
		error_fun(")", "comment end found without matching comment begin!");
	}, true },
	{ ":", "-- ; begins a user-supplied word definition", []() {
		if (state.compiling) {
			error_fun(":", "new words may only be defined while interpreting a line");
		}

		get_word();
		if (state.interp.word.len == 0) {
			error_fun(":", "expected word name");
		}

		state.comp = CompilerState {
			.code_start_idx = state.code_len,
			.name = state.interp.word,
			.desc_start = 0,
			.desc_len = 0,
		};
		state.compiling = true;

		get_word();
		if (state.interp.word.len != 1 || state.line[state.interp.word.pos] != '(') {
			state.interp.line_pos = state.interp.word.pos; // TODO: check that this is correct and shouldn't be -1
			return;
		}

		get_word();
		if (state.interp.word.len == 0) {
			error_fun(":", "expected matching ) for start of description");
		}
		state.comp.desc_start = state.interp.word.pos;
		state.interp.line_pos = state.interp.word.pos; // TODO: check that this is correct and shouldn't be -1

		int nesting = 1;
		while (nesting) {
			state.comp.desc_len = state.interp.word.pos + state.interp.word.len - state.comp.desc_start;

			get_word();
			if (state.interp.word.len == 0) {
				error_fun(":", "expected matching ) for start of description");
			}

			if (state.interp.word.len == 1 && state.line[state.interp.word.pos] == '(') {
				++nesting;
			} else if (state.interp.word.len == 1 && state.line[state.interp.word.pos] == ')') {
				--nesting;
			}
		}
	}, true },
	{ ";", "-- ; ends a user-supplied word definition", []() {
		if (!state.compiling) {
			error_fun(";", "a word definition may only be ended after it has been started");
		}

		if (state.words_len == MAX_WORDS) {
			error_fun(";", "too many words defined");
		}
		if (state.word_descs_len + state.comp.desc_len + 1 > DESC_BUF_LEN) {
			error_fun(";", "ran out of space for word description");
		}
		if (state.word_names_len + state.comp.name.len + 1 > NAME_BUF_LEN) {
			error_fun(";", "ran out of space for word name");
		}

		char *desc = &state.word_descs[state.word_descs_len];
		memcpy(
			desc,
			&state.line[state.comp.desc_start],
			state.comp.desc_len
		);
		desc[state.comp.desc_len] = 0;
		state.word_descs_len += state.comp.desc_len + 1;

		char *name = &state.word_names[state.word_names_len];
		memcpy(
			name,
			&state.line[state.comp.name.pos],
			state.comp.name.len
		);
		name[state.comp.name.len] = 0;
		state.word_names_len += state.comp.name.len + 1;

		state.words[state.words_len++] = Word {
			.name = name,
			.desc = desc,
			.code_pos = state.comp.code_start_idx,
			.code_len = state.code_len - state.comp.code_start_idx,
		};

		state.compiling = false;
	}, true },
	{ NULL, NULL, NULL },
};
constexpr size_t primitives_len = sizeof(primitives)/sizeof(*primitives) - 1;
#undef error_fun
#undef error

int32_t search_primitive(const char *name, size_t name_len) {
	if (name_len == 0) return -1;

	for (uint32_t i = 0; i < primitives_len; ++i) {
		if (strlen(primitives[i].name) != name_len) continue;

		bool matches = true;
		for (size_t j = 0; j < name_len; ++j) {
			if (primitives[i].name[j] != name[j]) {
				matches = false;
				break;
			}
		}

		if (matches) {
			return i;
		}
	}

	return -1;
}
int32_t search_word(const char *name, size_t name_len) {
	if (name_len == 0) return -1;

	uint32_t i = state.words_len;
	while (i --> 0) {
		if (strlen(state.words[i].name) != name_len) continue;

		bool matches = true;
		for (size_t j = 0; j < name_len; ++j) {
			if (state.words[i].name[j] != name[j]) {
				matches = false;
				break;
			}
		}

		if (matches) {
			return i;
		}
	}

	return -1;
}

#ifdef TRACING
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif
void run_compiled(const char *word_name) {
	TRACE("[(%s]", word_name);
	while (state.interp.code.idx < state.interp.code.len && state.interp.err == NULL) {
		const CodeElem head = state.code[state.interp.code.pos + state.interp.code.idx];
		++state.interp.code.idx;
		if (head.prefix == CodeElemPrefix::Literal) {
			if (state.interp.code.idx >= state.interp.code.len) {
				state.interp.err = "Error in compiled word: expected literal value";
				break;
			}
			if (state.stack_len >= STACK_SIZE) {
				state.interp.err = "Error at number literal: stack length should be < STACK_SIZE";
				break;
			}
			const uint32_t lit = state.code[state.interp.code.pos + state.interp.code.idx].lit;
			++state.interp.code.idx;
			TRACE("<(%u:%u>", lit, state.stack_len);
			stack_push(lit);
			TRACE("<%u:%u)>", lit, state.stack_len);
		} else if (head.prefix == CodeElemPrefix::Word) {
			if (state.interp.code.idx >= state.interp.code.len) {
				state.interp.err = "Error in compiled word: expected word index";
				break;
			}
			const uint32_t word_idx = state.code[state.interp.code.pos + state.interp.code.idx].idx;
			++state.interp.code.idx;
			const Word &word = state.words[word_idx];
			const auto save_state = state.interp.code;

			state.interp.code = CodePos {
				.pos = word.code_pos,
				.idx = 0,
				.len = word.code_len
			};
			TRACE("<(%s!%u>", word.name, state.stack_len);
			run_compiled(word.name);
			TRACE("<%s!%u)>", word.name, state.stack_len);
			state.interp.code = save_state;
		} else {
			const uint32_t prim_idx = head.idx;
			const PrimitiveEntry &prim = primitives[prim_idx];
			TRACE("<(%s$%u>", prim.name, state.stack_len);
			prim.func();
			TRACE("<%s$%u)>", prim.name, state.stack_len);
		}
	}
	TRACE("[%s)]", word_name);

	if (state.interp.err) {
		const auto _ = sdk::ColorSwitch(vga::Color::LightRed);

		if (!state.interp.err_handled) {
			putchar('\n');
			puts(state.interp.err);
		}
		printf("@ compiled word `%s`\n", word_name);

		state.interp.err_handled = true;
	}
}

void input_key(ps2::Key, char ch, bool capitalise) {
	if (state.line_len == MAX_LINE_LEN) {
		state.has_inp_err = true;
		state.inp_err_until = pit::millis + 100;

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
	state.line[state.line_len++] = ch;
	putchar(ch);

	if (state.line_len == MAX_LINE_LEN) term::cursor::disable();
}

void get_word() {
	size_t &start = state.interp.line_pos;
	size_t word_end = 0;

	while (start < state.line_len && state.line[start] == ' ') {
		++start;
	}
	word_end = start;
	while (word_end < state.line_len && state.line[word_end] != ' ') {
		++word_end;
	}

	state.interp.word.len = word_end - start;
	state.interp.word.pos = start;
	start += state.interp.word.len;
}

void interpret_line() {
	state.interp = {};

	size_t &word = state.interp.word.pos;
	size_t &len = state.interp.word.len;

	while (word < state.line_len && state.interp.err == NULL) {
		get_word();
		if (len == 0) break;

		const int32_t word_idx = search_word(&state.line[word], len);
		if (word_idx != -1) {
			state.interp.code = {
				.pos = state.words[word_idx].code_pos,
				.idx = 0,
				.len = state.words[word_idx].code_len,
			};
			if (state.compiling) {
				state.code[state.code_len++] = CodeElem {
					.prefix = CodeElemPrefix::Word,
				};
				state.code[state.code_len++] = CodeElem {
					.idx = *(uint32_t*)&word_idx,
				};
			} else {
				run_compiled(state.words[word_idx].name);
			}

			continue;
		}

		const int32_t prim_idx = search_primitive(&state.line[word], len);
		if (prim_idx != -1) {
			if (state.compiling && !primitives[prim_idx].immediate) {
				state.code[state.code_len++] = CodeElem {
					.idx = *(uint32_t*)&prim_idx,
				};
			} else {
				primitives[prim_idx].func();
			}

			continue;
		}

		bool is_number = true;
		uint32_t number = 0;
		for (size_t i = 0; i < len; ++i) {
			const char ch = state.line[word+i];
			if (ch < '0' || ch > '9') {
				is_number = false;
				break;
			} else {
				number *= 10;
				number += ch - '0';
			}
		}

		if (is_number) {
			if (state.stack_len >= STACK_SIZE) {
				state.interp.err = "Error at number literal: stack length should be < STACK_SIZE";
			} else if (state.compiling) {
				state.code[state.code_len++] = CodeElem {
					.prefix = CodeElemPrefix::Literal,
				};
				state.code[state.code_len++] = CodeElem {
					.lit = number,
				};
			} else {
				state.stack[state.stack_len++] = number;
			}
		} else {
			state.interp.err = "Error: Undefined word";
		}
	}

	if (state.interp.err) {
		const auto _ = sdk::ColorSwitch(vga::Color::LightRed);

		if (!state.interp.err_handled) {
			putchar('\n');
			puts(state.interp.err);
		}
		if (state.interp.word.len == 0) {
			puts("@ end of line");
		} else {
			printf("@ word starting at %u: ", state.interp.word.pos);
			for (size_t i = 0; i < state.interp.word.len; ++i) {
				putchar(state.line[state.interp.word.pos + i]);
			}
			putchar('\n');
		}

		state.interp.err_handled = true;
		state.compiling = false;
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
			|| state.capslock;
		input_key(key, key_ascii_map[key], capitalise);
		return;
	}

	if (key == KEY_BACKSPACE) {
		if (state.line_len == 0) return;

		if (state.has_inp_err) {
			state.has_inp_err = false;

			term::advance();
			term::backspace();
		}

		--state.line_len;
		term::backspace();
		term::cursor::enable(8, 15);
	}

	if (key == KEY_ENTER) {
		if (state.has_inp_err) {
			state.has_inp_err = false;

			term::advance();
			term::backspace();
		}

		putchar('\n');
		interpret_line();
		state.line_len = 0;
		term::writestring("> ");
		term::cursor::enable(8, 15);
		return;
	}
}

}

void main() {
	assert(!forth_running);
	forth_running = true;
	state = State{};

	term::clear();
	term::go_to(0, 0);
	puts("Enter `guide` for instructions on usage, or `exit` to exit the program.");
	term::writestring("> ");

	uint32_t start_pos, len;

	start_pos = state.code_len;
	state.code[state.code_len++] = { .idx = 11 };
	state.code[state.code_len++] = { .idx = 4 };
	state.code[state.code_len++] = { .idx = 14 };
	len = state.code_len - start_pos;
	const uint32_t minus_idx = state.words_len;
	state.words[state.words_len++] = { "-", "a b -- a-b", start_pos, len };

	start_pos = state.code_len;
	state.code[state.code_len++] = { .prefix = CodeElemPrefix::Literal };
	state.code[state.code_len++] = { .lit = 0 };
	state.code[state.code_len++] = { .idx = 1 };
	state.code[state.code_len++] = { .prefix = CodeElemPrefix::Word };
	state.code[state.code_len++] = { .idx = minus_idx };
	len = state.code_len - start_pos;
	const uint32_t neg_idx = state.words_len;
	state.words[state.words_len++] = { "_", "a -- -a", start_pos, len };
	(void) neg_idx;

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

	forth_running = false;
}

}
