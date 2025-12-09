#include "apps/forth.hpp"

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <sdk/terminal.hpp>
#include <sdk/util.hpp>

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
struct RawFunction {
	const char *name;
	void(*func)();
};
static_assert(sizeof(RawFunction*) == sizeof(uint32_t), "Expected pointers to be 32 bits");

struct CompilerState {
	size_t code_start_idx;
	WordPos name;
	size_t desc_start;
	size_t desc_len;
};

enum class CodeElemPrefix {
	Literal = -1,
	Word = -2,
	RawFunc = -3,
};
static_assert(sizeof(CodeElemPrefix) == sizeof(uint32_t), "Expected enum class to be 32 bits");
union CodeElem {
	CodeElemPrefix prefix;
	uint32_t lit;
	uint32_t idx;
	RawFunction *fun;
};
static_assert(sizeof(CodeElem) == sizeof(uint32_t), "Expected union of 32-bit values to be 32 bits");

constexpr size_t MAX_LINE_LEN = vga::WIDTH - 3;
constexpr size_t LINE_BUF_LEN = 128; // in order to support more complex pre-defined words
static_assert(LINE_BUF_LEN >= MAX_LINE_LEN, "a line should be able to contain at least MAX_LINE_LEN characters");
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
	char line[LINE_BUF_LEN];
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
	int skip = 0;
	InterpreterState interp;
	CompilerState comp;

	State(const State&) = delete;
	State &operator=(const State&) = delete;
	State(State&&) = default;
	State &operator=(State&&) = default;
};
static State state{};
bool forth_running = false;

using namespace sdk::util;

Maybe<uint32_t> compile_word(uint32_t index);
Maybe<uint32_t> compile_primitive(uint32_t index);
Maybe<uint32_t> compile_number(uint32_t num);
Maybe<uint32_t> compile_raw_func(RawFunction *func);
void run_compiled_section(uint32_t len);
void run_word(uint32_t index);
void run_primitive(uint32_t index);
void run_number(uint32_t num);
void run_raw_func(RawFunction *func);
Maybe<uint32_t> parse_word(const char *str, size_t len);
Maybe<uint32_t> parse_primitive(const char *str, size_t len);
Maybe<uint32_t> parse_number(const char *str, size_t len);
Maybe<uint32_t> read_compiled_word(CodePos &pos);
Maybe<uint32_t> read_compiled_primitive(CodePos &pos);
Maybe<uint32_t> read_compiled_number(CodePos &pos);
Maybe<RawFunction *> read_compiled_raw_func(CodePos &pos);

enum class ValueType {
	Word,
	Primitive,
	Number,
	RawFunction,
};
struct Value {
	ValueType type;
	union {
		uint32_t word;
		uint32_t primitive;
		uint32_t number;
		RawFunction *raw_function;
	};

	static Maybe<Value> parse(const char *str, size_t len) {
		const auto word = parse_word(str, len);
		if (word.has) return { {
			.type = ValueType::Word,
			.word = word.get(),
		} };
		const auto primitive = parse_primitive(str, len);
		if (primitive.has) return { {
			.type = ValueType::Primitive,
			.primitive = primitive.get(),
		} };
		const auto number = parse_number(str, len);
		if (number.has) return { {
			.type = ValueType::Number,
			.number = number.get(),
		} };
		return {};
	}
	static Maybe<Value> read_compiled(CodePos &pos) {
		const auto word = read_compiled_word(pos);
		if (word.has) return { {
			.type = ValueType::Word,
			.word = word.get(),
		} };
		const auto primitive = read_compiled_primitive(pos);
		if (primitive.has) return { {
			.type = ValueType::Primitive,
			.primitive = primitive.get(),
		} };
		const auto number = read_compiled_number(pos);
		if (number.has) return { {
			.type = ValueType::Number,
			.number = number.get(),
		} };
		const auto raw_func = read_compiled_raw_func(pos);
		if (raw_func.has) return { {
			.type = ValueType::RawFunction,
			.raw_function = raw_func.get(),
		} };
		return {};
	}

	inline Maybe<uint32_t> compile() const {
		switch (type) {
			case ValueType::Word: return compile_word(word);
			case ValueType::Primitive: return compile_primitive(primitive);
			case ValueType::Number: return compile_number(number);
			case ValueType::RawFunction: return compile_raw_func(raw_function);
			default: assert(false && "unreachable");
		}
	}
	inline void run() const {
		switch (type) {
			case ValueType::Word: run_word(word); break;
			case ValueType::Primitive: run_primitive(primitive); break;
			case ValueType::Number: run_number(number); break;
			case ValueType::RawFunction: run_raw_func(raw_function); break;
		}
	}
};

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
#define check_stack_cap(fun, expr) if (state.stack_len + (expr) >= STACK_SIZE) error_fun(fun, "stack capacity should be at least " #expr)
#define check_code_len(fun, len) if (state.code_len + (len) > CODE_BUF_SIZE) error_fun(fun, "not enough space to generate code for user word")
RawFunction print_raw = { "<internal:print_raw>", []() {
	check_stack_len_ge("<internal:print_raw>", 1);
	const char *str = reinterpret_cast<const char*>(stack_pop());
	term::writestring(str);
} };
void print_definition(uint32_t word_idx);
RawFunction raw_print_definition = { "<internal:print_definition>", []() {
	check_stack_len_ge("<internal:print_definition>", 1);
	print_definition(stack_pop());
} };
RawFunction recurse = { "rec", []() {
	state.interp.code.idx = 0;
} };
RawFunction rf_return = { "ret", []() {
	state.interp.code.idx = state.interp.code.len;
} };
const PrimitiveEntry primitives[] = {
	/* STACK OPERATIONS */
	{ ".", "-- ; shows the top 16 elements of the stack", []() {
		if (state.stack_len == 0) { puts("empty."); return; }

		const size_t amt = state.stack_len < 16 ? state.stack_len : 16;
		if (state.stack_len > 16) term::writestring("... ");
		for (size_t i = amt; i > 0; --i) {
			printf("%d ", stack_peek(i));
		}
		putchar('\n');
	} },
	{ "stack_len", "-- a ; pushes length of stack", []() {
		check_stack_len_lt("stack_len", STACK_SIZE);
		stack_push(state.stack_len);
	} },
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
	{ "unrot", "a b c -- c a b", []() {
		check_stack_len_ge("rot", 3);
		const uint32_t c = stack_pop();
		const uint32_t b = stack_pop();
		const uint32_t a = stack_pop();
		stack_push(c);
		stack_push(a);
		stack_push(b);
	} },
	{ "rev", "a b c -- c b a", []() {
		check_stack_len_ge("rot", 3);
		const uint32_t c = stack_pop();
		const uint32_t b = stack_pop();
		const uint32_t a = stack_pop();
		stack_push(c);
		stack_push(b);
		stack_push(a);
	} },
	{ "drop", "a --", []() {
		check_stack_len_ge("drop", 1);
		stack_pop();
	} },
	{ "rev_n", "... n -- ... ; reverse the top n elements", []() {
		check_stack_len_ge("rot_n", 1);
		const uint32_t n = stack_pop();
		check_stack_len_ge("rot_n", n);
		for (size_t i = 0; i < n/2; ++i) {
			const size_t fst_ix = i+1;
			const size_t scd_ix = n-i;
			const uint32_t tmp = stack_peek(fst_ix);
			stack_get(fst_ix) = stack_peek(scd_ix);
			stack_get(scd_ix) = tmp;
		}
	} },
	{ "nth", "... n -- ... x ; dup the nth element down to the top", []() {
		check_stack_len_ge("nth", 1);
		const uint32_t n = stack_pop();
		check_stack_len_ge("nth", n);
		if (n == 0) {
			error_fun("nth", "n must be nonzero");
		}
		stack_push(stack_peek(n));
	} },

	/* ARYTHMETIC OPERATIONS */
	{ "inc", "a -- a+1", []() {
		check_stack_len_ge("inc", 1);
		++stack_get();
	} },
	{ "dec", "a -- a-1", []() {
		check_stack_len_ge("dec", 1);
		--stack_get();
	} },
	{ "+", "a b -- a+b", []() {
		check_stack_len_ge("+", 2);
		stack_push(stack_pop() + stack_pop());
	} },
	{ "*", "a b -- a*b", []() {
		check_stack_len_ge("*", 2);
		stack_push(stack_pop() * stack_pop());
	} },
	{ "/", "a b -- a/b", []() {
		check_stack_len_ge("/", 2);
		const uint32_t b = stack_pop();
		const uint32_t a = stack_pop();
		stack_push(a / b);
	} },

	/* BITWISE OPERATIONS */
	{ "shl", "a b -- a<<b", []() {
		check_stack_len_ge("shl", 2);
		const uint32_t top = stack_pop();
		const uint32_t under_top = stack_pop();
		if (top >= 32) {
			stack_push(0);
		} else {
			stack_push(under_top << top);
		}
	} },
	{ "shr", "a b -- a>>b", []() {
		check_stack_len_ge("shr", 2);
		const uint32_t top = stack_pop();
		const uint32_t under_top = stack_pop();
		if (top >= 32) {
			stack_push(0);
		} else {
			stack_push(under_top >> top);
		}
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

	/* COMPARISON */
	{ "=", "a b -- a=b", []() {
		check_stack_len_ge("=?", 2);
		stack_push(stack_pop() == stack_pop() ? ~0 : 0);
	} },
	{ "<", "a b -- a<b", []() {
		check_stack_len_ge("=?", 2);
		const uint32_t b_raw = stack_pop();
		const uint32_t a_raw = stack_pop();
		const int32_t b = *(int32_t*)&b_raw;
		const int32_t a = *(int32_t*)&a_raw;
		stack_push(a < b ? ~0 : 0);
	} },

	/* LITERALS */
	{ "true", "-- -1", []() {
		check_stack_len_lt("true", STACK_SIZE);
		stack_push(~0);
	} },
	{ "false", "-- 0", []() {
		check_stack_len_lt("false", STACK_SIZE);
		stack_push(0);
	} },
	{ "hex", "-- a ; interprets next word as hex number and pushes it", []() {
		if (!state.compiling && state.skip) {
			get_word();
			return;
		}
		if (!state.compiling) check_stack_len_lt("hex", STACK_SIZE);
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
			check_code_len("hex", 2);

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
		if (!state.compiling && state.skip) {
			get_word();
			return;
		}
		if (!state.compiling) check_stack_len_lt("'", STACK_SIZE);
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
			check_code_len("'", 2);

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

	/* OUTPUT OPERATIONS */
	{ "print", "a -- ; prints top element of stack as a signed number", []() {
		check_stack_len_ge("print", 1);
		printf("%d ", reinterpret_cast<uint32_t>(stack_pop()));
	} },
	{ "pstr", "a -- ; prints top element as string of at most four characters", []() {
		check_stack_len_ge("pstr", 1);
		const uint32_t str_raw = stack_pop();
		const char *str = (char*)&str_raw;
		for (size_t i = 0; i < 4; ++i) {
			if (str[i] == 0) break;
			term::putchar(str[i]);
		}
	} },

	/* STRINGS */
	{ "\"", "-- ... n ; pushes a string to the top of the stack (data then length)", []() {
		WordPos &word = state.interp.word;

		if (!state.compiling && state.skip) {
			get_word();

			while (word.len != 0 && !(word.len == 1 && state.line[word.pos] == '"')) {
				get_word();
			}

			if (word.len == 0) {
				error_fun("\"", "expected closing \" for string");
			}

			return;
		}

		size_t start = 0;
		size_t end = 0;
		get_word();
		start = word.pos;

		while (word.len != 0 && !(word.len == 1 && state.line[word.pos] == '"')) {
			end = word.pos + word.len;
			get_word();
		}

		if (word.len == 0) {
			error_fun("\"", "expected closing \" for string");
		}

		const size_t len = end - start;
		const size_t words = len/4 + !!(len&3);

		if (state.compiling) {
			check_code_len("\"", (words+1)*2);
		} else {
			check_stack_cap("\"", words+1);
		}

		for (size_t i = 0; i < words; ++i) {
			uint32_t num = 0;
			size_t j = i*4 + 4;
			if (j > len) j = len;
			while (j --> i*4) {
				const char ch = state.line[start + j];
				num <<= 8;
				num |= *(const uint8_t*)&ch;
			}
			if (state.compiling) {
				state.code[state.code_len++] = CodeElem {
					.prefix = CodeElemPrefix::Literal,
				};
				state.code[state.code_len++] = CodeElem {
					.lit = num,
				};
			} else {
				stack_push(num);
			}
		}
		if (state.compiling) {
			state.code[state.code_len++] = CodeElem {
				.prefix = CodeElemPrefix::Literal,
			};
			state.code[state.code_len++] = CodeElem {
				.lit = words,
			};
		} else {
			stack_push(words);
		}
	}, true },
	{ "print_string", "... n -- ; prints a string of length n", []() {
		check_stack_len_ge("print_string", 1);
		const uint32_t n = stack_pop();

		check_stack_len_ge("print_string", n);
		term::writestring((const char*)&state.stack[state.stack_len - n]);
		state.stack_len -= n;
	} },

	/* SYSTEM OPERATION */
	{ "exit", "-- ; exits the forth interpreter", []() {
		state.should_quit = true;
	} },
	{ "quit", "-- ; exits the forth interpreter", []() {
		state.should_quit = true;
	} },
	// TODO: sleep functions perhaps, clearing the keyboard buffer when done? Essentially ignoring all user input while sleeping

	/* DOCUMENTATION / HELP / INSPECTION */
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
	{ "help", "-- ; prints help text for the next word", []() {
		if (!state.compiling && state.skip) {
			get_word();
			return;
		}

		get_word();
		if (state.interp.word.len == 0) {
			error_fun("help", "expected following word");
		}

		const auto word_idx = parse_word(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		);
		if (word_idx.has && state.compiling) {
			check_code_len("help", 15);

			const char *s = "`: ";

			assert(compile_number('`').get() == 2);
			assert(compile_primitive(parse_primitive(
				"pstr", 4
			).get()).get() == 1);

			assert(compile_number(
				reinterpret_cast<uint32_t>(state.words[word_idx.get()].name)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			assert(compile_number(
				reinterpret_cast<uint32_t>(s)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			assert(compile_number(
				reinterpret_cast<uint32_t>(state.words[word_idx.get()].desc)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			return;
		} else if (word_idx.has) {
			const auto &word = state.words[word_idx.get()];
			printf("`%s`: %s", word.name, word.desc);

			return;
		}

		const auto prim_idx = parse_primitive(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		);
		if (prim_idx.has && state.compiling) {
			check_code_len("help", 15);

			const char *s = "`: ";

			assert(compile_number('`').get() == 2);
			assert(compile_primitive(parse_primitive(
				"pstr", 4
			).get()).get() == 1);

			assert(compile_number(
				reinterpret_cast<uint32_t>(primitives[prim_idx.get()].name)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			assert(compile_number(
				reinterpret_cast<uint32_t>(s)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			assert(compile_number(
				reinterpret_cast<uint32_t>(primitives[prim_idx.get()].desc)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			return;
		} else if (prim_idx.has) {
			const auto &prim = primitives[prim_idx.get()];
			printf("`%s`: %s", prim.name, prim.desc);

			return;
		}

		error_fun("help", "Couldn't find specified word");
	}, true },
	{ "def", "-- ; prints the definition of a given word", []() {
		if (!state.compiling && state.skip) {
			get_word();
			return;
		}

		get_word();
		if (state.interp.word.len == 0) {
			error_fun("def", "expected following word");
		}

		const auto word_idx = parse_word(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		);
		if (word_idx.has && state.compiling) {
			check_code_len("def", 4);

			assert(compile_number(word_idx.get()).get() == 2);
			assert(compile_raw_func(&raw_print_definition).get() == 2);

			return;
		} else if (word_idx.has) {
			print_definition(word_idx.get());

			return;
		}

		const auto prim_idx = parse_primitive(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		);
		if (prim_idx.has && state.compiling) {
			check_code_len("def", 12);

			const char *s0 = "<built-in primitive `";
			const char *s1 = "`>";

			assert(compile_number(
				reinterpret_cast<uint32_t>(s0)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			assert(compile_number(
				reinterpret_cast<uint32_t>(primitives[prim_idx.get()].name)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			assert(compile_number(
				reinterpret_cast<uint32_t>(s1)
			).get() == 2);
			assert(compile_raw_func(&print_raw).get() == 2);

			return;
		} else if (prim_idx.has) {
			printf(
				"<built-in primitive `%s`>",
				primitives[prim_idx.get()].name
			);

			return;
		}

		error_fun("def", "Couldn't find specified word");
	}, true },
	{ "guide", "-- ; prints usage guide for the forth interpreter", []() {
		const char *guide_text =
			"This is a FORTH interpreter. It is operated by entering a sequence of space-seperated words into the prompt.\n"
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

	/* INTERNALS / SYNTAX */
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

		if (state.skip) {
			WordPos &word = state.interp.word;

			get_word();

			while (word.len != 0 && !(word.len == 1 && state.line[word.pos] == ';')) {
				get_word();
			}

			if (word.len == 0) {
				error_fun(":", "expected closing ; for word definition");
			}

			return;
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
	{ "rec", "-- ; recurses (runs the current word from the start)", []() {
		if (!state.compiling) {
			error_fun("rec", "rec is only valid when defining a word (inside : ; )");
		}
		check_code_len("rec", 2);

		state.code[state.code_len++] = {
			.prefix = CodeElemPrefix::RawFunc,
		};
		state.code[state.code_len++] = {
			.fun = &recurse
		};
	}, true },
	{ "ret", "-- ; returns (exits the current word early)", []() {
		if (!state.compiling) {
			error_fun("ret", "ret is only valid when defining a word (inside : ; )");
		}
		check_code_len("ret", 2);

		state.code[state.code_len++] = {
			.prefix = CodeElemPrefix::RawFunc,
		};
		state.code[state.code_len++] = {
			.fun = &rf_return
		};
	}, true },
	{ "?", "a -- ; only executes the next word if the stack top is nonzero", []() {
		if (!state.compiling) {
			if (state.skip) {
				++state.skip;
				return;
			}

			check_stack_len_ge("?", 1);

			if (stack_pop() == 0) ++state.skip;

			return;
		}

		check_code_len("?", 4);

		static RawFunction compiled = { "?", []() {
			const uint32_t next_len = read_compiled_number(state.interp.code).get();

			check_stack_len_ge("?", 1);

			if (stack_pop() == 0) ++state.skip;

			run_compiled_section(next_len);
		} };

		assert(compile_raw_func(&compiled).get() == 2);
		assert(compile_number(0).get() == 2);
		uint32_t &size = state.code[state.code_len-1].lit;

		get_word();
		size = Value::parse(
			&state.line[state.interp.word.pos],
			state.interp.word.len
		).get().compile().get();
	}, true },
	{ "rep_and", "n -- ??? n ; repeat the next word n times, and push n to the stack", []() {
		if (!state.compiling) {
			if (state.skip) {
				++state.skip;
				return;
			}

			check_stack_len_ge("rep_and", 1);
			const size_t n = stack_pop();

			size_t &word = state.interp.word.pos;
			size_t &len = state.interp.word.len;

			if (n == 0) {
				++state.skip;
				while (word < state.line_len && state.interp.err == NULL && state.skip != 0) {
					get_word();
					if (len == 0) break; // TODO: should I error here?

					const auto value = Value::parse(&state.line[word], len);
					if (value.has) {
						value.get().run();
					} else {
						error_fun("rep_and", "undefined word");
					}
				}

				if (!state.interp.err) {
					check_stack_cap("rep_and", 1);
					stack_push(n);
				}
			} else {
				get_word();
				if (len == 0) error_fun("rep_and", "expected following word");

				const auto value = Value::parse(&state.line[word], len);
				if (value.has) {
					const auto raw_value = value.get();
					const auto save_state = state.interp;
					for (uint32_t i = 0; i < n; ++i) {
						state.interp = save_state;
						raw_value.run();
						if (state.interp.err) break;
					}

					if (!state.interp.err) {
						check_stack_cap("rep_and", 1);
						stack_push(n);
					}
				} else {
					error_fun("rep_and", "undefined word");
				}
			}
		} else {
			check_code_len("rep_and", 4);

			static RawFunction compiled = { "rep_and", []() {
				const uint32_t next_len = read_compiled_number(state.interp.code).get();

				check_stack_len_ge("rep_and", 1);
				const size_t n = stack_pop();

				if (n == 0) {
					++state.skip;

					run_compiled_section(next_len);
					return;
				}

				const auto save_state = state.interp.code;
				for (uint32_t i = 0; i < n; ++i) {
					state.interp.code = save_state;
					run_compiled_section(next_len);
					if (state.interp.err) break;
				}

				if (!state.interp.err) {
					check_stack_cap("rep_and", 1);
					stack_push(n);
				}
			} };

			assert(compile_raw_func(&compiled).get() == 2);
			assert(compile_number(0).get() == 2);
			uint32_t &size = state.code[state.code_len-1].lit;

			get_word();
			size = Value::parse(
				&state.line[state.interp.word.pos],
				state.interp.word.len
			).get().compile().get();
		}
	}, true },
	{ "rep", "n -- ??? ; repeat the next word n times", []() {
		if (!state.compiling && state.skip) {
			primitives[parse_primitive("rep_and", 7).get()].func();
			return;
		}

		primitives[parse_primitive("rep_and", 7).get()].func();

		if (state.compiling) {
			check_code_len("rep", 1);

			assert(compile_primitive(
				parse_primitive("drop", 4).get()
			).get() == 1);
		} else {
			check_stack_len_ge("rep", 1);
			stack_pop();
		}
	}, true },

	{ NULL, NULL, NULL },
};
constexpr size_t primitives_len = sizeof(primitives)/sizeof(*primitives) - 1;
void print_definition(uint32_t word_idx) {
	printf(
		": %s ( %s )",
		state.words[word_idx].name,
		state.words[word_idx].desc
	);
	// no checking as to validity
	CodePos pos = {
		.pos = state.words[word_idx].code_pos,
		.idx = 0,
		.len = state.words[word_idx].code_len,
	};
	while (pos.idx < pos.len) {
		const auto value = Value::read_compiled(pos).get();
		switch (value.type) {
			case ValueType::Word: {
				printf(" %s", state.words[value.word].name);
			} break;
			case ValueType::Primitive: {
				printf(" %s", primitives[value.primitive].name);
			} break;
			case ValueType::Number: {
				printf(" %u", value.number);
			} break;
			case ValueType::RawFunction: {
				printf(" %s", value.raw_function->name);
			} break;
		}
	}
	printf(" ;");
}

/*RawFunction repeat = { "rep_and", []() {
	check_stack_len_ge("rep_and", 1);
	const size_t n = stack_pop();

	const auto value = Value::read_compiled(state.interp.code);
	if (value.has) {
		for (uint32_t i = 0; i < n; ++i) {
			value.get().run();
		}

		if (!state.interp.err) {
			stack_push(n);
		}
	} else {
		error_fun("rep_and", "expected value");
	}
} };*/

#undef error_fun
#undef error

Maybe<uint32_t> compile_word(uint32_t index) {
	if (state.code_len + 2 > CODE_BUF_SIZE) {
		state.interp.err = "not enough space to compile word";
		return {};
	}

	state.code[state.code_len++] = {
		.prefix = CodeElemPrefix::Word,
	};
	state.code[state.code_len++] = {
		.idx = index,
	};

	return 2;
}
Maybe<uint32_t> compile_primitive(uint32_t index) {
	const PrimitiveEntry &prim = primitives[index];

	if (prim.immediate) {
		const uint32_t curr = state.code_len;
		prim.func();
		if (state.interp.err) {
			state.code_len = curr;
			return {};
		} else return state.code_len - curr;
	}

	if (state.code_len + 1 > CODE_BUF_SIZE) {
		state.interp.err = "not enough space to compile primitive";
		return {};
	}

	state.code[state.code_len++] = {
		.idx = index,
	};

	return 1;
}
Maybe<uint32_t> compile_number(uint32_t num) {
	if (state.code_len + 2 > CODE_BUF_SIZE) {
		state.interp.err = "not enough space to compile number";
		return {};
	}

	state.code[state.code_len++] = {
		.prefix = CodeElemPrefix::Literal,
	};
	state.code[state.code_len++] = {
		.lit = num,
	};

	return 2;
}
Maybe<uint32_t> compile_raw_func(RawFunction *func) {
	if (state.code_len + 2 > CODE_BUF_SIZE) {
		state.interp.err = "not enough space to compile internal function";
		return {};
	}

	state.code[state.code_len++] = {
		.prefix = CodeElemPrefix::RawFunc,
	};
	state.code[state.code_len++] = {
		.fun = func,
	};

	return 2;
}
#ifdef TRACING
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif
void run_compiled_section(uint32_t len) {
	const uint32_t begin = state.interp.code.idx;
	const uint32_t end = begin + len;

	assert(end < state.interp.code.pos + state.interp.code.len); // TODO: proper error reporting

	while (state.interp.code.idx < end && state.interp.err == NULL) {
		const auto value = Value::read_compiled(state.interp.code);
		if (value.has) {
			if (!state.compiling && state.skip) ++state.skip;
			value.get().run();
		} else {
			state.interp.err = "Error in compiled word: failed to read valid code";
		}
	}

	if (!state.compiling && state.skip) --state.skip;
}
void run_word(uint32_t index) {
	const auto &word = state.words[index];

	if (state.skip) {
		--state.skip;
		return;
	}

	const auto save_state = state.interp.code;
	state.interp.code = {
		.pos = word.code_pos,
		.idx = 0,
		.len = word.code_len,
	};

	TRACE("[.%s]", word.name);
	run_compiled_section(word.code_len);
	TRACE("[%s.]", word.name);

	state.interp.code = save_state;

	if (state.interp.err) {
		const auto _ = sdk::ColorSwitch(vga::Color::LightRed);

		if (!state.interp.err_handled) {
			putchar('\n');
			puts(state.interp.err);
		}
		printf("@ compiled word `%s`\n", word.name);

		state.interp.err_handled = true;
	}
}
#undef TRACE
void run_primitive(uint32_t index) {
	if (state.skip) {
		if (primitives[index].immediate) primitives[index].func();
		--state.skip;
	} else primitives[index].func();
}
void run_number(uint32_t num) {
	if (state.skip) {
		--state.skip;
		return;
	}

	if (state.stack_len + 1 > STACK_SIZE) {
		state.interp.err = "not enough stack space to push number";
		return;
	}

	stack_push(num);
}
void run_raw_func(RawFunction *func) {
	if (state.skip) {
		--state.skip;
		return;
	}
	func->func();
}
Maybe<uint32_t> parse_word(const char *str, size_t len) {
	if (len == 0) return {};

	uint32_t i = state.words_len;
	while (i --> 0) {
		if (strlen(state.words[i].name) != len) continue;

		bool matches = true;
		for (size_t j = 0; j < len; ++j) {
			if (state.words[i].name[j] != str[j]) {
				matches = false;
				break;
			}
		}

		if (matches) {
			return i;
		}
	}

	return {};
}
Maybe<uint32_t> parse_primitive(const char *str, size_t len) {
	if (len == 0) return {};

	for (uint32_t i = 0; i < primitives_len; ++i) {
		if (strlen(primitives[i].name) != len) continue;

		bool matches = true;
		for (size_t j = 0; j < len; ++j) {
			if (primitives[i].name[j] != str[j]) {
				matches = false;
				break;
			}
		}

		if (matches) {
			return i;
		}
	}

	return {};
}
Maybe<uint32_t> parse_number(const char *str, size_t len) {
	uint32_t res = 0;
	for (size_t i = 0; i < len; ++i) {
		const uint32_t prev = res;
		if (str[i] < '0' || str[i] > '9') return {};

		res *= 10;
		res += str[i] - '0';

		if (res < prev) {
			// overflow detected!
			// TODO: do something?
		}
	}
	return res;
}
Maybe<uint32_t> read_compiled_word(CodePos &pos) {
	if (pos.idx + 2 > pos.len) return {};
	const CodeElemPrefix pref = state.code[pos.pos + pos.idx].prefix;
	if (pref != CodeElemPrefix::Word) return {};
	const uint32_t res = state.code[pos.pos + pos.idx + 1].idx;
	pos.idx += 2;
	return res;
}
Maybe<uint32_t> read_compiled_primitive(CodePos &pos) {
	if (pos.idx + 1 > pos.len) return {};
	const CodeElemPrefix pref = state.code[pos.pos + pos.idx].prefix;
	if (pref == CodeElemPrefix::Literal) return {};
	if (pref == CodeElemPrefix::Word) return {};
	if (pref == CodeElemPrefix::RawFunc) return {};
	const uint32_t res = state.code[pos.pos + pos.idx].idx;
	pos.idx += 1;
	return res;
}
Maybe<uint32_t> read_compiled_number(CodePos &pos) {
	if (pos.idx + 2 > pos.len) return {};
	const CodeElemPrefix pref = state.code[pos.pos + pos.idx].prefix;
	if (pref != CodeElemPrefix::Literal) return {};
	const uint32_t res = state.code[pos.pos + pos.idx + 1].lit;
	pos.idx += 2;
	return res;
}
Maybe<RawFunction *> read_compiled_raw_func(CodePos &pos) {
	if (pos.idx + 2 > pos.len) return {};
	const CodeElemPrefix pref = state.code[pos.pos + pos.idx].prefix;
	if (pref != CodeElemPrefix::RawFunc) return {};
	RawFunction *const res = state.code[pos.pos + pos.idx + 1].fun;
	pos.idx += 2;
	return res;
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

		const auto value = Value::parse(&state.line[word], len);
		if (value.has) {
			if (state.compiling) {
				value.get().compile();
			} else {
				value.get().run();
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

#define run_line(code) do { \
		state.line_len = strlen(code); \
		assert(state.line_len < LINE_BUF_LEN); \
		memcpy(state.line, (code), state.line_len); \
		interpret_line(); \
		state.line_len = 0; \
	} while (0)

void main() {
	assert(!forth_running);
	forth_running = true;
	state = State{};

	run_line(": - ( a b -- a-b ) not inc + ;");
	run_line(": neg ( a -- -a ) 0 swap - ;");

	run_line(": *_under ( a b -- a a*b ) swap dup rot * ;");
	run_line(": ^ ( a b -- a^b ; a to the power b ) 1 swap rep *_under swap drop ;");

	run_line(": != ( a b -- a!=b ) = not ;");
	run_line(": <= ( a b -- a<=b ) dup rot dup rot < unrot = or ;");
	run_line(": >= ( a b -- a>=b ) < not ;");
	run_line(": > ( a b -- a>=b ) <= not ;");

	run_line(": truthy? ( a -- a!=false ) false != ;");

	run_line(": show_top ( a -- a ; prints the topmost stack element ) dup print ;");
	run_line(": clear ( ... - ; clears the stack ) stack_len 0 = ? ret drop rec ;");

	term::clear();
	term::go_to(0, 0);
	puts("Enter `guide` for instructions on usage, or `exit` to exit the program.");
	term::writestring("> ");

	run_line("\" Tests for rep_and: \" print_string");
	run_line("1 0 rep ? help rep . clear");
	run_line("0 1 rep ? help rep . clear");
	run_line(": \255 rep ? help rep ; def \255");
	run_line("1 0 \255 . clear");
	run_line("0 1 \255 . clear");

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
